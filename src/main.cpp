#include <iostream>
#include <iomanip>
#include <chrono>

#include <arrow/api.h>
#include <arrow/ipc/api.h>
#include <arrow/io/api.h>

/**
 * Builds the schema that we expect for the input record batch.
 */
std::shared_ptr<arrow::Schema> schema_in() {
  return arrow::schema({arrow::field("text", arrow::utf8(), false)});
}

/**
 * Builds the schema that we will use for the output record batch.
 */
std::shared_ptr<arrow::Schema> schema_out() {
  return arrow::schema({arrow::field("match", arrow::uint32(), false)});
}

/**
 * Reads a record batch file.
 */
std::shared_ptr<arrow::RecordBatch> ReadBatch(const std::string &file_name) {
  auto file = arrow::io::ReadableFile::Open(file_name).ValueOrDie();
  auto reader = arrow::ipc::RecordBatchFileReader::Open(file).ValueOrDie();
  auto batch = reader->ReadRecordBatch(0);
  return batch.ValueOrDie();
}

/**
 * Writes a record batch file.
 */
arrow::Status WriteBatch(const std::string &file_name, const arrow::RecordBatch &batch) {
  arrow::Status status;
  auto file = arrow::io::FileOutputStream::Open(file_name).ValueOrDie();
  auto writer = arrow::ipc::NewFileWriter(file.get(), batch.schema()).ValueOrDie();
  status = writer->WriteRecordBatch(batch);
  if (!status.ok()) {
    return status;
  }
  return writer->Close();
}

/**
 * Application entry point.
 */
int main(int argc, char **argv) {

  // Check number of input args.
  if (argc != 3) {
    std::cout << "Usage: " << argv[0] << " <input.rb> <output.rb>" << std::endl;
    return 2;
  }

  arrow::Status status;

  // Read batch from file.
  std::cout << "Loading dataset..." << std::endl;
  auto batch_in = ReadBatch(argv[1]);

  // Check if input schema matches (except metadata).
  batch_in->schema()->Equals(schema_in(), false);

  // Cast to utf8 string array.
  auto array = std::dynamic_pointer_cast<arrow::StringArray>(batch_in->column(0));

  if (array == nullptr) {
    std::cerr << "Could not cast Array to StringArray" << std::endl;
    return 1;
  }

  // Obtain the number of rows, number of data bytes, and the raw buffer
  // pointers.
  size_t num_rows = array->length();
  const auto *offsets = reinterpret_cast<const int32_t *>(array->value_offsets()->data());
  size_t num_bytes = offsets[num_rows] - offsets[0];
  const auto *values = reinterpret_cast<const uint8_t *>(array->value_data()->data());
  std::cout << "Loaded dataset with " << num_rows << " row(s) and " << num_bytes << " data byte(s)." << std::endl;

  // Make an output buffer that's large enough to handle the case where all
  // records match.
  uint32_t *matches = nullptr;
  const size_t matches_size = num_rows * sizeof(uint32_t);
  posix_memalign(&matches, 64, matches_size);
  size_t num_matches = 0;

  // Connect to the FPGA.
  std::shared_ptr<Tidre> t;
  auto status = Tidre::Make(
    &t, "aws",
    10, // Number of pipeline beats; tweakable, at least 1.
    8,  // Number of kernels to use; tweakable from 1 to 8.
    2,  // DDR bank to use for even beats, must be 2 or 3.
    3   // DDR bank to use for odd beats, must be 2 or 3.
  );
  if (!status.ok()) {
    std::cerr << "Could not connect to FPGA: " << status.message << std::endl;
    return 1;
  }

  // Process the dataset with the FPGA.
  std::cout << "Starting FPGA run for " << num_rows << " rows..." << std::endl;
  size_t num_errors = 0;
  auto start = std::chrono::system_clock::now();
  status = t->RunRaw(
    offsets, values, num_rows, matches, matches_size, &num_matches, &num_errors,
    0   // Verbosity level; 0, 1, or 2.
  );
  auto end = std::chrono::system_clock::now();
  if (!status.ok()) {
    std::cerr << "Failed to run on FPGA: " << status.message << std::endl;
    return 1;
  }

  // Print some results.
  auto time = std::chrono::duration<double>(end - start).count();
  std::cout << "Run complete in " << std::fixed << std::setprecision(4) << time << " seconds, ";
  std::cout << "or about " << (num_bytes / time) * 1e-9 << " GB/s" << std::endl;
  std::cout << num_matches << " match(es) were recorded." << std::endl;
  if (num_errors) {
    std::cout << "Note: there was/were " << num_errors << " UTF8 decode error(s) ";
    std::cout << "while reading the dataset!" << std::endl;
  }

  // Wrap the resulting buffer in an output record batch.
  size_t num_matches_bytes = num_matches * sizeof(uint32_t);
  auto matches_values_buffer = arrow::Buffer::Wrap(matches, num_matches_bytes);
  auto matches_array = std::make_shared<arrow::PrimitiveArray>(
    arrow::uint32(), num_matches, matches_values_buffer);
  auto batch_out = arrow::RecordBatch::Make(schema_out(), num_matches, {matches_array});

  // Write the record batch to a file.
  std::cout << "Writing match data..." << std::endl;
  status = WriteBatch(argv[2], *batch_out);
  if (!status.ok()) {
    std::cout << "Failed to write output record batch: " << status.message() << std::endl;
  }

  return 0;
}
