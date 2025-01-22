Bluegrass Rust
==============

Quickstart:

    rustup target add thumbv7m-none-eabi
    
Over STlink or other probe:

    # see https://probe.rs/docs/getting-started/installation/
    cargo install probe-rs-tools --locked
    
    # possibly install udev rules, see
    # https://probe.rs/docs/getting-started/probe-setup/#st-link
    
    cargo run --release

Over USB bootloader:

    # I would like to use cargo-dfu, but it doesn't understand our bootloader
    
    sudo apt-get install dfu-util
    cargo install cargo-binutils
    rustup component add llvm-tools
    
    cargo objcopy --release -- -O binary program.bin
    cargo size --release
    
    dfu-util -w -d 1eaf:0003 -a 2 -D program.bin
    # press reset button on device
