# Hệ Thống Điều Khiển Nút Bấm & LED trên Raspberry Pi 4 Model B

Dự án Lập trình Nhúng Linux xây dựng hệ thống điều khiển **Nút Bấm và LED** hoàn chỉnh trên nền tảng **Raspberry Pi 4 Model B (ARM64)**.

Thay vì điều khiển GPIO đơn giản từ Userspace Polling qua sysfs/libgpiod, dự án này triển khai theo 3 tầng:
1. **Tầng Kernel Space:** Trực tiếp viết Platform Driver độc lập, tích hợp vào Device Tree, xử lý Hardware Interrupts, cơ chế Non-blocking/Sleep bằng Wait Queue và nhúng thẳng vào nhân Linux (Built-in Driver).
2. **Tầng Middleware:** Đóng gói thành các share lib (`libled.so`, `libbutton.so`) để trừu tượng hóa phần cứng.
3. **Tầng Userspace:** Ứng dụng điều khiển đa luồng xử lý logic bấm nhả / bấm giữ và được cấu hình chạy tự động lúc khởi động bằng `systemd`.

🔌 Sơ Đồ Đấu Nối Phần Cứng

| Linh kiện   | Chân vật lý (Pin) | Broadcom GPIO | Chế độ hoạt động     | Ghi chú                                                                |
| :---------- | :---------------- | :------------ | :------------------- | :--------------------------------------------------------------------- |
| **LED (+)** | Pin 40            | `GPIO 21`     | Output (Active High) | Nối tiếp qua điện trở 220Ω - 330Ω về cực âm                            |
| **Nút bấm** | Pin 38            | `GPIO 20`     | Input (Active Low)   | Cấu hình **Điện trở kéo lên nội bộ (Internal Pull-Up)** bằng `pinctrl` |
| **GND (-)** | Pin 39 / Pin 34   | `GND`         | Nối đất              | Chân Mass chung cho toàn hệ thống                                      |

✨ Các Kỹ Thuật Nổi Bật Đã Ứng Dụng

1. Tầng Kernel Space & BSP

  - GPIO Descriptor Subsystem: Sử dụng API người tiêu dùng mới nhất
    (devm_gpiod_get()) thay cho API số nguyên cũ đã lỗi thời.
  - Device Tree & Pin Control: Tạo mới các node phần cứng độc lập
    (clean,custom-led và clean,custom-button), cấu hình Kéo trở lên thông qua
    pinctrl trên file gốc bcm2711-rpi-4-b.dts.
  - Ngắt phần cứng (Hardware IRQ): Bắt chính xác sự kiện thay đổi trạng thái
    chân bằng ngắt sườn (IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING) với hàm
    devm_request_threaded_irq().
  - Cơ chế Hàng đợi Chờ (Wait Queue): Sử dụng wait_event_interruptible() và
    wake_up_interruptible() để tiến trình đọc rơi vào trạng thái ngủ sâu (tiêu
    tốn 0% CPU) khi nút bấm chưa được nhấn.
  - Nhúng thẳng Driver vào Kernel (Built-in Driver): Tích hợp vào hệ thống
    Kconfig và Kbuild (CONFIG_BUTTON_LED=y), dịch thẳng vào file nhân Image
    (không cần dùng module rời .ko).

2. Tầng Middleware / Shared Libraries

  - Độc lập và module hóa chức năng bằng thư viện chia sẻ liên kết động (.so).
  - libled: Cung cấp bộ API điều khiển LED rõ ràng: led_init(), led_set(),
    led_toggle(), led_close().
  - libbutton: Đọc dữ liệu từ tầng Kernel ('P' - Bấm, 'R' - Nhả), tính toán
    khoảng thời gian bấm (CLOCK_MONOTONIC), chống rung phím (Debounce) và phát
    ra các sự kiện cấp cao thông qua con trỏ hàm Callback.

3. Tầng Userspace & Init System

  - Phân loại hành vi nhấn:
      - Bấm nhả nhanh (50ms < t < 1000ms): Bật hoặc Tắt đèn (Toggle ON/OFF).
      - Bấm giữ lâu (≥ 1000ms): Kích hoạt luồng chạy ngầm điều khiển LED nhấp
        nháy chu kỳ 200ms.
  - An toàn Đa luồng (Thread-safety): Dùng pthread_mutex_t để ngăn chặn xung đột
    tài nguyên giữa luồng giao diện sự kiện và luồng nháy LED ngầm.
  - Tự động hóa: Thiết lập systemd service (button-led.service) tự động khởi
    chạy ứng dụng cùng hệ thống khi Pi bật nguồn, tự phục hồi khi gặp sự cố
    (Restart=always).

🎮 Kết Quả

  - Bấm nhả nút nhanh: Đèn LED đổi trạng thái (Đang Tắt -> Sáng, Đang Sáng ->
    Tắt).
  - Bấm và giữ nút trên 1 giây rồi nhả: Đèn LED tự động chuyển sang chế độ nhấp
    nháy liên tục với chu kỳ 200ms.
  - Khi đang nháy, bấm nhả nhanh một cái: Lập tức hủy chế độ nháy và tắt đèn
    ngay lập tức.
  - Tự khởi động: Thử rút nguồn Pi và cắm lại, toàn bộ hệ thống sẽ tự vận hành ngay khi Pi boot xong mà không cần đăng nhập SSH hay can thiệp bàn phím.

