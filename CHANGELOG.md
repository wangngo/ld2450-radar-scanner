# CHANGELOG

## 2026-11-05
- Sửa theo review advisor: gộp <1m mặc định TẮT; fetch timeout 1s + xoá mục tiêu khi mất kết nối (tránh báo người giả);
  Hz tính từ số frame radar; `Serial.setTxTimeoutMs(0)`; END cfg gửi lại mỗi 5s khi radar im; lọc y<=0;
  ghim `espressif32@6.9.0` (= Arduino core 2.0.17, docs cũ ghi nhầm 3.x).
- Lúc boot: hỏi chế độ radar (0x0091), chuyển multi-target nếu cần; lọc 0,1–6m; gộp mục tiêu <1m (bật/tắt trên web qua `/cfg`). Test native 12/12.
- Web: thống kê bám liên tục/mất dấu, vòng mờ khi mất dấu, làm mượt EMA.
- Bản web: ESP32-S3 phát WiFi AP, trang radar trên điện thoại (poll /data 10Hz), env `s3_web` RX=18/TX=17.
- Thêm vào plan: bíp theo khoảng cách, chọn tầm 3/4/6m, v2 cảm biến đặt xa qua ESP-NOW (tham khảo dự án LD2450+LoRa+Pi).
- Khởi tạo repo: parser LD2450 + test native, firmware v0 in CSV, docs/PLAN.md.
