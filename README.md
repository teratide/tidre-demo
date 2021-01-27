Tidre demo
==========

Tidre is an FPGA accelerator for matching regular expressions, created by
[Teratide](https://teratide.io/). The regex is baked into the bitstream, but
everything can be automatically generated from a given regex, and once you
have the bitstream, you can match faster than the PCIe link to the FPGA can
keep up with.

The bitstream generator is closed source at this time, but we do provide two
demo bitstreams for evaluation purposes via Amazon AWS. These are:

 - `agfi-095f5880469ff5b8e`: matches `.*[Tt][Aa][Xx][Ii].*`.
 - `agfi-028fcd53df79020d9`: matches `.*[tT][eE][rR][aA][tT][iI][dD][eE][ \t\n]+[dD][iI][vV][iI][nN][gG][ \t\n]+([sS][uU][bB])+[sS][uU][rR][fF][aA][cC][eE].*`.

Both have the same performance -- the FPGA design doesn't care about the
complexity of the regular expression, as long as it meets timing. So far it
has for every regular expression we've tried.

This repository contains what you need to try either of these bitstreams out
for yourself. You just need an AWS account with the ability to launch F1
instances. You may need to request a quota limit increase from AWS to do so,
but this doesn't take long in our experience.

Step 1: launch the F1 instance
------------------------------

Go [here](https://console.aws.amazon.com/ec2/v2/home?region=us-east-1#LaunchInstanceWizard:)
to start the process. If the link is dead, just look for the "launch instance"
button on your EC2 dashboard. Keep in mind that not all regions have F1
instances!

In step 1 (AMI selection), look for AWS' CentOS-based FPGA Developer AMI on the
marketplace. We've tested this with version 1.9.1. In step 2 (instance type),
select `f1.2xlarge`. In step 4 (storage), you can get rid of the `/dev/sdb`
drive or select "delete on termination" if you like; we won't be using that
part of the image.

For the remaining steps, this guide assumes you're SSH'd into your F1 instance.

Step 2: download and install dependencies
-----------------------------------------

First of all, you'll need this repository. It uses submodules; make sure to do
a recursive clone.

```
git clone https://github.com/teratide/tidre-demo.git --recursive
cd tidre-demo
```

We'll get Arrow as a prebuilt package. This just follows Arrow's own
installation instructions. We also install cmake3 here, which we need for the
build systems.

```
sudo yum install -y https://apache.bintray.com/arrow/centos/7/apache-arrow-release-latest.rpm
sudo yum install -y cmake3 arrow-devel-1.0.1-1.el7
```

Next, we'll need to build Fletcher. The correct version of Fletcher is
available as a submodule in `fletcher-aws/fletcher`. Just build and install as
per the norm for any CMake project.

```
cd fletcher-aws/fletcher
mkdir -p build
cd build
cmake3 ..
make -j
sudo make install
sudo ldconfig
cd ../../..
```

Fletcher on its own has no platform support; support for specific FPGA
platforms needs to be installed separately. The support library for AWS can be
found at `fletcher-aws/runtime/runtime`. Before we can build that, however,
we'll need some environment setup from AWS.

```
source fletcher-aws/aws-fpga/sdk_setup.sh
```

After that, just install normally.

```
cd fletcher-aws/runtime/runtime
mkdir -p build
cd build
cmake3 ..
make -j
sudo make install
sudo ldconfig
cd ../../../..
```

In order to communicate with the FPGA, we'll also need the XDMA driver. It can
be installed as follows.

```
cd fletcher-aws/aws-fpga/sdk/linux_kernel_drivers/xdma
make -j
sudo make install
cd ../../../../..
```

For the data generator, we'll also need some Python dependencies.

```
sudo pip3 install --upgrade pip
sudo pip3 install pyarrow==1.0.1 rstr==2.2.6
```

Step 3: prepare a dataset
-------------------------

In order to run something, you'll of course also need a dataset. The demo
application expects an Arrow record batch with a single `UTF8` column, and
outputs a record batch with a single `uint32` column.

To get you going, an example data generator is provided as a Python script. By
default it will generate a dataset with strings matching both the "taxi" and
"Teratide diving subsurface" regular expressions (with different frequencies),
about 1GB in size. You can run it as follows.

```
python3 data-gen.py
```

This generates a file named `input.rb`.

While the script doesn't take any command-line arguments, you can easily
reconfigure it inside the script.

Step 4: load the FPGA image
---------------------------

Now let's get the FPGA going. Before you can use it, you have to clear it.

```
sudo fpga-clear-local-image -S 0
```

Now you can load either of the bitstreams (run ONE of these):

```
sudo fpga-load-local-image -S 0 -I agfi-095f5880469ff5b8e  # taxi
OR
sudo fpga-load-local-image -S 0 -I agfi-028fcd53df79020d9  # teratide diving subsurface
```

That should be everything. You don't have to clear again if you want to change
the bitstream.

Step 5: build and run the demo application
------------------------------------------

Now that we have all our dependencies, we can build the demo application at the
root of the tidre-demo repository.

```
mkdir -p build
cd build
cmake3 .. -DCMAKE_INSTALL_PREFIX=/usr
make
sudo make install
cd ..
```

You can then run the application as follows.

```
sudo tidre-demo input.rb output.rb
```
