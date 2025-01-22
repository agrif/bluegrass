#![no_std]
#![no_main]

use defmt_rtt as _;
use panic_probe as _;

use stm32f1xx_hal as hal;

use hal::pac;
use hal::prelude::*;
use usb_device::prelude::*;

#[cortex_m_rt::entry]
fn main() -> ! {
    let cp = cortex_m::Peripherals::take().unwrap();
    let dp = pac::Peripherals::take().unwrap();

    use hal::rcc::Reset;
    hal::pac::USB::reset(&dp.RCC);

    let mut flash = dp.FLASH.constrain();
    let rcc = dp.RCC.constrain();

    // set up the clocks. unfortately, we can't get a 72MHz sysclk without an
    // HSE predivider
    // https://github.com/stm32-rs/stm32f1xx-hal/issues/448
    // so we make do with a 48 MHz sysclk
    let clocks = rcc
        .cfgr
        .use_hse(16.MHz())
        .sysclk(48.MHz())
        .freeze(&mut flash.acr);

    defmt::assert!(clocks.usbclk_valid());

    let mut delay = hal::timer::Timer::syst(cp.SYST, &clocks).counter_us();

    let mut gpioa = dp.GPIOA.split();
    let mut gpioc = dp.GPIOC.split();

    // pull d+ down to force re-enumeration
    let mut usb_dp = gpioa.pa12.into_push_pull_output(&mut gpioa.crh);
    usb_dp.set_low();
    delay.start(10.millis()).unwrap();
    nb::block!(delay.wait()).unwrap();

    let usb_bus = hal::usb::UsbBus::new(hal::usb::Peripheral {
        usb: dp.USB,
        pin_dm: gpioa.pa11,
        pin_dp: usb_dp.into_floating_input(&mut gpioa.crh),
    });

    let mut serial = usbd_serial::SerialPort::new(&usb_bus);

    let mut usb_dev = UsbDeviceBuilder::new(&usb_bus, UsbVidPid(0x16c0, 0x27dd))
        .manufacturer("Fake Company")
        .product("Bluegrass rust-blink")
        .serial_number("TEST")
        .device_class(usbd_serial::USB_CLASS_CDC)
        .build();

    // LED is active low
    let mut led = gpioc.pc13.into_push_pull_output(&mut gpioc.crh);

    // systick is too fine at this speed to do 500millis in one go
    let mut delay_state = 0;
    delay.start(100.millis()).unwrap();

    loop {
        usb_dev.poll(&mut [&mut serial]);

        if let Ok(()) = delay.wait() {
            delay_state += 1;
        }

        if delay_state > 5 {
            delay_state = 0;

            if led.is_set_high() {
                defmt::println!("LED On.");
                let _ = serial.write(b"LED On.\r\n");
                led.set_low();
            } else {
                defmt::println!("LED Off.");
                let _ = serial.write(b"LED Off.\r\n");
                led.set_high();
            }
        }
    }
}
