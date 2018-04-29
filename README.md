![](banner.png)

MicroJIT is a Just-In-Time compiler for Alex Roger's [Stack][stack] Virtual
Machine on the [BBC micro:bit][microbit]. This repository contains all the
software required to run the JIT compiler on a micro:bit and deploy Stack
programs to it via a USB serial connection. You can see a [video demonstration
here][video].

[stack]: http://www.cs.ox.ac.uk/people/alex.rogers/stack/
[microbit]: http://microbit.org
[video]: https://youtu.be/JEjn7r80kkw

The JIT compiler is written in C++ and generates Arm Thumb bytecode. Compiled
programs are written to the micro:bit's flash, so you can reboot the device and
run the same program &mdash; once the currently stored program has finished
running you can deploy a new one, using the script described below.

## Building & using

*Please note that the following has only been tested under macOS 10.13.*

To get started, you'll need to configure your computer for [offline micro:bit
development][offline]. Please also ensure that you have Python 3+ installed.
Clone this repository and run `scripts/init.sh` from the root directory of the
repository.  This will download all the dependencies of the project and build
the binary that you need to deploy to the micro:bit.

[offline]: https://lancaster-university.github.io/microbit-docs/offline-toolchains/#installation-on-mac-osx

Once the binary is compiled, either copy
`build/bbc-microbit-classic-gcc/source/microjit-combined.hex` to your micro:bit
or run `scripts/deploy.sh` (again, from the root directory). If you make any
changes to the code you can also use `scripts/deploy.sh` to rebuild and deploy
the code to the binary.

The micro:bit software supports receiving new Stack programs via serial. If you
have a file containing Stack code encoded in hexadecimal, run
`scripts/send_stack.py -a [filename]`. The `-a` flag indicates that the file is
encoded in ASCII hexadecimal; if you have a binary file instead run
`scripts/send_stack.py [filename]`. Run `scripts/send_stack.py --help` to see
more options.

Please note that this script closes the serial connection when it terminates.
Closing the connection causes the micro:bit to reboot, which will mean your
program restarts. You can pass a flag `-t [seconds]` to `scripts/send_stack.py`
that will optionally pause the script, giving the micro:bit sufficient time to
run your program before rebooting. This also has the annoying effect of running
your program twice.

You may prefer to use an adapted version of the [Stack website][website] running
locally on your computer. This uses the same code for serial transmission, but
keeps the serial connection open whilst the web server is running. It also
allows you to use Alex Roger's programming environment directly with the
micro:bit.

[website]: https://github.com/thomasdenney/stack-site

## Running tests

There are around 400 unit tests that run on the micro:bit. To test the project,
change `Mode` in `source/Config.h` to `ProjectMode::UnitTests` or
`ProjectMode::OptionalInstructionTests`.

## License & contributing

The contents of this repository is available under the MIT License. The Stack
Virtual Machine is &copy; Alex Rogers 2017.

The contents of this repository represents all the code that I wrote for my
undergraduate thesis; as such I cannot accept pull requests until *after* the
examiners have had the opportunity to look at it (July 2018).

