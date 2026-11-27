# WE3 Redux — External / English Commentary Pack Guide

`SoundFX` (`scripts/autoload/sound_fx.gd`) tự động quét và nạp các file âm thanh (`.wav`, `.ogg`, `.mp3`) đặt trong thư mục `assets/audio/commentary/`.

## 1. Cấu trúc thư mục
- **Câu bình luận tình huống (tiếng Anh / gói ngoài)**:
  Đặt file vào `assets/audio/commentary/en/<event>/` (có thể đặt nhiều file trong 1 thư mục, game sẽ chọn ngẫu nhiên):
  - `en/kickoff/` — giao bóng bắt đầu trận / hiệp
  - `en/goal/` — ghi bàn
  - `en/shot/` — dứt điểm
  - `en/save/` — thủ môn cản phá
  - `en/post/` — bóng dội cột dọc / xà ngang
  - `en/foul/` — thổi phạt
  - `en/yellow_card/` — thẻ vàng
  - `en/red_card/` — thẻ đỏ
  - `en/offside/` — việt vị
  - `en/corner/` — phạt góc
  - `en/throw_in/` — ném biên
  - `en/free_kick/` — đá phạt
  - `en/penalty/` — phạt đền
  - `en/halftime/` — hết hiệp 1
  - `en/fulltime/` — hết trận
  - `en/setpiece/` — fallback chung cho tình huống cố định

- **Tên cầu thủ (`players/`)**:
  Đặt file vào `assets/audio/commentary/players/<ten_cau_thu>.wav` (viết thường, dấu cách thay bằng `_`, ví dụ `ronaldo.wav`, `zidane.ogg`, `del_piero.wav`).
  - Khi ghi bàn: game phát `[event_goal]` nối tiếp `[ten_cau_thu]` (VD: *"GOAL! ... Ronaldo!"*).
  - Khi sút hoặc dính thẻ: game phát `[ten_cau_thu]` nối tiếp `[event]` (VD: *"Ronaldo ... shoots!"*).

## 2. Cơ chế ưu tiên & Fallback
- Nếu thư mục `en/<event>/` có ít nhất 1 file âm thanh, `SoundFX` **tự động ưu tiên dùng gói tiếng Anh** thay cho clip tiếng Nhật `isspro*.wav`.
- Nếu thư mục `en/<event>/` chưa có file, `SoundFX` tự động fallback về clip tiếng Nhật gốc (hoặc im lặng với event chưa có clip gốc).
