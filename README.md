# Hệ thống robot vận chuyển hàng trong kho sử dụng STM32

## 1. Giới thiệu dự án

Dự án hướng đến xây dựng một robot vận chuyển kiện hàng nhỏ giữa các trạm trong mô hình kho hàng. Robot sử dụng vi điều khiển STM32 để tiếp nhận yêu cầu, điều khiển động cơ, di chuyển theo bản đồ đã khai báo và gửi trạng thái về máy tính.

Phạm vi thiết kế dự kiến gồm một robot, một trạm xuất hàng S và ba trạm giao hàng A, B, C. Robot chở một kiện hàng mỗi chuyến. Sau khi đến trạm đích và hoàn tất bàn giao, robot quay về S để chờ nhiệm vụ tiếp theo.

Dự án cần đáp ứng các yêu cầu của bài tập lớn: làm việc trực tiếp với vi xử lý, điều khiển thiết bị vào/ra, thiết kế và thuyết minh mạch, sử dụng RTOS, kết nối và điều khiển từ máy tính, hiển thị trạng thái hệ thống, demo trên thiết bị thật và cung cấp hướng dẫn chạy mã nguồn.

> **Lưu ý về trạng thái:** README này phân biệt các phần đã được xác nhận trong quá trình làm việc với các chức năng mới ở mức thiết kế hoặc chưa được kiểm thử. Không xem các chức năng trong kế hoạch là đã hoàn thành.

## 2. Nền tảng và thành phần đã xác định

- **Vi điều khiển:** STM32F103C8T6.
- **Môi trường mô phỏng đã sử dụng:** Proteus.
- **Driver động cơ:** TB6612FNG.
- **Giao tiếp máy tính:** UART qua USART1.
- **Chân USART1 đã sử dụng:** PA9 (TX), PA10 (RX).
- **Thuật toán tìm đường dự kiến:** BFS trên bản đồ cố định 4×4.
- **Cơ chế điều hướng mục tiêu:** Robot chạy theo tuyến đường đã xác định; thiết kế đầy đủ còn cần chốt cơ chế nhận biết giao lộ và vị trí robot.

Các linh kiện đầu vào/đầu ra khác như cảm biến dò vạch, cảm biến vật cản, nút nhấn, màn hình, cảm biến có hàng và mạch nguồn/dao động/reset riêng nằm trong thiết kế dự kiến. Chưa có đủ thông tin xác nhận rằng tất cả đã được lắp ráp và kiểm thử.

## 3. Bản đồ kho hàng dự kiến

```text
S ─── P ─── Q ─── A
│     │     │     │
R ─── T ─── U ─── V
│     │     │     │
W ─── B ─── X ─── Y
│     │     │     │
Z ─── M ─── N ─── C
```

Đánh số theo thứ tự từng hàng, từ trái sang phải:

| Hàng | Nút và chỉ số |
|---|---|
| 1 | S = 0, P = 1, Q = 2, A = 3 |
| 2 | R = 4, T = 5, U = 6, V = 7 |
| 3 | W = 8, B = 9, X = 10, Y = 11 |
| 4 | Z = 12, M = 13, N = 14, C = 15 |

Các đường nối ngang và dọc biểu diễn những đoạn đường có thể đi giữa các nút kề nhau. Đây là bản đồ logic dự kiến; cần bảo đảm nó khớp với đường vạch trên mô hình vật lý khi triển khai bám vạch.

## 4. Lệnh giao hàng đã thống nhất

| Lệnh UART | Nhiệm vụ dự kiến |
|---|---|
| `A` | Đi từ S đến A, sau đó quay về S |
| `B` | Đi từ S đến B, sau đó quay về S |
| `C` | Đi từ S đến C, sau đó quay về S |

## 5. Những phần đã thực hiện được

Các mục dưới đây là những nội dung đã được ghi nhận trong quá trình làm việc:

- Đã làm việc với mô hình STM32F103C8T6 trong Proteus.
- Đã sử dụng driver TB6612FNG trong hệ thống điều khiển động cơ.
- Đã thiết lập giao tiếp UART qua USART1, sử dụng PA9 (TX) và PA10 (RX).
- Đã sửa vấn đề liên quan đến hàm ngắt nhận UART.
- Đã xử lý định dạng hàng đợi/lệnh và phần ghi log.
- Đã kiểm tra được hiện tượng bánh xe quay khi hệ thống nhận lệnh đích.

**Giới hạn xác nhận:** Những kết quả trên chưa đủ để kết luận rằng toàn bộ robot đã tìm đúng đường, nhận biết giao lộ, giao hàng, xử lý sự cố và tự quay về S thành công.

## 6. Thiết kế chức năng và trạng thái hiện tại

| Hạng mục | Trạng thái hiện tại |
|---|---|
| STM32F103C8T6 và Proteus | Đã làm việc trong môi trường mô phỏng |
| Giao tiếp UART USART1 | Đã cấu hình và đã xử lý lỗi nhận UART |
| Nhận lệnh A/B/C | Đã thống nhất giao thức; cần kiểm thử lại toàn bộ trường hợp |
| Điều khiển động cơ | Đã quan sát bánh xe quay; cần kiểm tra hướng, tốc độ, rẽ và dừng |
| Bản đồ 4×4 | Đã xác định bản đồ logic và chỉ số các nút |
| BFS tìm đường | Là thuật toán được chọn cho thiết kế; cần xác minh mã nguồn và kết quả thực thi |
| Điều hướng theo giao lộ | Chưa xác nhận hoàn thiện |
| Bám vạch bằng cảm biến | Nằm trong thiết kế đề xuất; chưa xác nhận đã triển khai |
| Phát hiện vật cản và lập tuyến thay thế | Chưa xác nhận đã triển khai |
| Nút nhấn, màn hình và xác nhận lấy/đặt hàng | Chưa xác nhận đã triển khai |
| RTOS | Chưa có thông tin xác nhận đã tích hợp và kiểm thử |
| Phần mềm máy tính có giao diện hiển thị trạng thái | UART đã được sử dụng; giao diện giám sát hoàn chỉnh chưa được xác nhận |
| Mạch STM32 trần với mạch nguồn, dao động, reset riêng | Là định hướng thiết kế; chưa xác nhận đã chế tạo và kiểm thử |
| Demo trên thiết bị thật | Chưa có thông tin xác nhận đã thực hiện |
| Báo cáo, 10 use case và hướng dẫn chạy mã nguồn | Cần hoàn thiện theo yêu cầu môn học |

## 7. 10 use case đề xuất

Các use case sau thuộc phạm vi chức năng dự kiến, không phải danh sách chức năng đã hoàn thành.

| Mã | Use case | Kết quả mong muốn |
|---|---|---|
| UC01 | Khởi động và chuẩn bị robot | Khởi tạo ngoại vi, hiệu chuẩn cảm biến nếu có và chuyển sang trạng thái sẵn sàng |
| UC02 | Tạo yêu cầu vận chuyển | Người vận hành chọn A/B/C và xác nhận nhiệm vụ |
| UC03 | Quản lý hàng đợi vận chuyển | Xem, xếp hàng và hủy nhiệm vụ chưa chạy |
| UC04 | Tiếp nhận hàng tại S | Robot chỉ khởi hành khi hàng đã được đặt và xác nhận |
| UC05 | Vận chuyển đến trạm đích | Tìm đường, điều khiển động cơ và dừng đúng trạm |
| UC06 | Bàn giao hàng | Chờ lấy hàng và xác nhận trước khi kết thúc nhiệm vụ |
| UC07 | Xử lý tuyến đường có vật cản | Dừng an toàn; chỉ tìm tuyến thay thế khi robot xác định được vị trí tại giao lộ |
| UC08 | Tạm dừng và tiếp tục nhiệm vụ | Dừng động cơ, giữ trạng thái và tiếp tục khi an toàn |
| UC09 | Khôi phục sau sự cố mất vạch | Dừng, báo lỗi và yêu cầu xác nhận/đặt lại vị trí trước khi chạy lại |
| UC10 | Quay về S và nhận nhiệm vụ tiếp theo | Tự trở về điểm xuất hàng và chờ nhiệm vụ tiếp theo |

## 8. Công việc cần hoàn thiện

### 8.1. Vi điều khiển và điều khiển vào/ra

- [ ] Rà soát mã nguồn hiện tại và xác minh chức năng UART nhận lệnh A/B/C.
- [ ] Kiểm tra GPIO, PWM/timer và các chân điều khiển TB6612FNG theo sơ đồ mạch thực tế.
- [ ] Kiểm tra riêng các thao tác tiến, lùi, rẽ trái, rẽ phải và dừng khẩn cấp.
- [ ] Đọc datasheet của STM32F103C8T6, TB6612FNG và từng linh kiện ngoại vi thực sự sử dụng.
- [ ] Tính toán và giải thích giá trị linh kiện trong báo cáo dựa trên datasheet, dòng điện, điện áp và công thức thiết kế; không dùng giá trị linh kiện tùy ý nếu chưa có tính toán.

### 8.2. Bản đồ, tìm đường và điều hướng

- [ ] Kiểm tra thuật toán BFS và đường đi trả về cho A, B, C.
- [ ] Xây dựng cách chuyển chuỗi nút thành lệnh điều khiển động cơ.
- [ ] Xác định cơ chế cập nhật hướng hiện tại và nhận biết giao lộ.
- [ ] Nếu triển khai bám vạch, kết nối và kiểm thử cảm biến dò vạch.
- [ ] Xác định cách ước lượng hoặc xác nhận vị trí nếu chưa có cảm biến định vị.
- [ ] Kiểm thử đầy đủ các tuyến đi và tuyến quay về S.

### 8.3. RTOS và quản lý nhiệm vụ

- [ ] Chọn và tích hợp một RTOS phù hợp với môi trường phát triển STM32.
- [ ] Phân chia tác vụ hợp lý, ví dụ: nhận UART/giao tiếp máy tính, điều khiển động cơ, đọc cảm biến và quản lý nhiệm vụ.
- [ ] Xác định cách trao đổi dữ liệu an toàn giữa tác vụ bằng queue, semaphore hoặc cơ chế đồng bộ phù hợp.
- [ ] Kiểm thử hàng đợi nhiệm vụ, tạm dừng/tiếp tục và xử lý lỗi.

### 8.4. Kết nối và phần mềm máy tính

- [ ] Hoàn thiện chương trình trên máy tính để gửi lệnh điều khiển.
- [ ] Hiển thị trạng thái robot, điểm đích, nhiệm vụ hiện tại và thông báo lỗi.
- [ ] Kiểm tra trường hợp mất kết nối UART hoặc nhận lệnh không hợp lệ.

### 8.5. Mạch và demo thiết bị thật

- [ ] Hoàn thiện sơ đồ nguyên lý và sơ đồ kết nối chân.
- [ ] Giải thích chức năng từng khối: nguồn, MCU, reset/clock, driver động cơ, cảm biến, nút nhấn và màn hình nếu được sử dụng.
- [ ] Kiểm tra nguồn logic và nguồn động cơ, dòng tải và nối mass chung theo thiết kế.
- [ ] Chế tạo/lắp ráp mạch và kiểm thử an toàn trước khi chạy robot.
- [ ] Thực hiện demo toàn bộ hành trình trên thiết bị thật và ghi lại kết quả.

### 8.6. Báo cáo

- [ ] Viết giới thiệu bài toán, mục tiêu và ý nghĩa.
- [ ] Mô tả 10 use case kèm luồng hoạt động và tiêu chí kiểm thử.
- [ ] Liệt kê linh kiện, phần mềm, hệ điều hành và hướng dẫn cài đặt.
- [ ] Trình bày thiết kế mạch, lý do chọn linh kiện và giải thích các giá trị linh kiện bằng tính toán/datasheet.
- [ ] Hướng dẫn chạy mã nguồn từng bước.
- [ ] Thêm ảnh mạch, sơ đồ, ảnh mô phỏng và kết quả demo rõ nét.
- [ ] Viết kết luận và hướng phát triển.
- [ ] Ghi rõ đóng góp và tỷ lệ đóng góp của từng thành viên theo yêu cầu giảng viên.

## 9. Nguyên tắc tiếp tục phát triển

1. Tiếp tục tập trung vào STM32F103C8T6 và Proteus; không tiếp tục Webots trừ khi có yêu cầu mới.
2. Trước khi sửa code, kiểm tra phiên bản mã nguồn hiện tại và giữ lại các phần đã hoạt động.
3. Hoàn thành từng chức năng nhỏ và kiểm thử trước khi tích hợp.
4. Phân biệt rõ kết quả đã kiểm thử với chức năng mới chỉ ở mức thiết kế.
5. Không khẳng định đã dùng RTOS hoặc demo thiết bị thật nếu chưa có bằng chứng thực tế.
