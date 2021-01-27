import pyarrow as pa
import rstr
import random

# Each tuple specifies a type of string to generate. The first entry specifies
# how many unique strings to generate (rstr is pretty slow). The second
# specifies how often to insert a string from that pool of unique strings into
# the actual dataset compared to inserting strings from the other pools. The
# third is the regex that the strings for this pool should match.
generator_config = [
    (100,  1, r'.*[tT][eE][rR][aA][tT][iI][dD][eE][ \t\n]+[dD][iI][vV][iI][nN][gG][ \t\n]+([sS][uU][bB])+[sS][uU][rR][fF][aA][cC][eE].*'),
    (100,  3, r'.*[Tt][Aa][Xx][Ii].*'),
    (300, 20, r'.*.*.*'), # long random strings
    (500, 20, r'.*'), # short random strings
]

# Target size for the dataset. Generation stops when either limit is reached.
target_num_rows  =    10_000_000
target_num_bytes = 1_000_000_000

# Construct pools of random strings abiding by the generator configuration.
random_strings = []
frequency_norm = 0
for _, frequency, _ in generator_config:
    frequency_norm += frequency
cumulative_frequency = 0.0
for num_unique, frequency, regex in generator_config:
    print('Creating random strings for /' + regex + '/...')
    cumulative_frequency += frequency / frequency_norm
    string_pool = [rstr.xeger(regex) for _ in range(num_unique)]
    random_strings.append((cumulative_frequency, string_pool))

# Construct the test data.
print('Constructing test data...')
data = []
total_len = 0
print()
while total_len < target_num_bytes and len(data) < target_num_rows:
    r = random.random()
    for cumulative_frequency, string_pool in random_strings:
        if r <= cumulative_frequency:
            s = random.choice(string_pool)
            total_len += len(s)
            data.append(s)
            break
    if len(data) % 1000 == 0:
        print('\033[A\033[K{:.1f}%...'.format(
            min(max(total_len / target_num_bytes, len(data) / target_num_rows) * 100, 100)))

# Write the generated data to a record batch.
print('Converting to record batch...')
field = pa.field('text', pa.utf8(), nullable=False)
schema = pa.schema([field])
arrays = [pa.array(data, pa.utf8())]
with pa.RecordBatchFileWriter('input.rb', schema) as writer:
    print('Writing file...')
    writer.write(pa.RecordBatch.from_arrays(arrays, schema=schema))
print('Done!')
