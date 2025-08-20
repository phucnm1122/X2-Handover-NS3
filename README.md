File gốc nằm ở đường dẫn sau

<pre> /ns-3.45/src/lte/examples/lena-x2-handover-measures.cc </pre>

📘 Hướng dẫn setup tham số cho file LenaX2HandoverMeasures.cc


1. Tham số chính trong mô phỏng

Bạn có thể điều chỉnh qua command line hoặc sửa trực tiếp trong code.

## 1. Tham số chính trong mô phỏng

| Tham số                 | Mặc định trong code | Ý nghĩa                                   |
|-------------------------|---------------------|-------------------------------------------|
| `numberOfUes`           | 1                   | Số lượng UE (user equipment)              |
| `numberOfEnbs`          | 3                   | Số lượng eNodeB                           |
| `numBearersPerUe`       | 1                   | Số lượng bearer mỗi UE                    |
| `distance`              | 200.0 m             | Khoảng cách giữa các eNB                  |
| `speed`                 | 16.4 m/s            | Vận tốc của UE (~60 km/h)                 |
| `simTime`               | `(numberOfEnbs+1) * distance / speed` | Thời gian mô phỏng (giây) |
| `enbTxPowerDbm`         | 46 dBm              | Công suất phát của eNB                    |
| `UdpClient::Interval`   | 2000 ms             | Chu kỳ gửi gói UDP                        |
| `UdpClient::MaxPackets` | 1.000.000           | Số gói UDP tối đa                         |
| `SrsPeriodicity`        | 2                   | Chu kỳ báo cáo SRS                        |
| `LteHelper::UseIdealRrc`| true                | Sử dụng Ideal RRC (đơn giản hóa)          |
| `HandoverAlgorithm`     | A3-RSRP             | Thuật toán handover mặc định              |
| `Hysteresis`            | 3 dB                | Ngưỡng hysteresis cho handover            |
| `TimeToTrigger`         | 256 ms              | Thời gian cần đạt để kích hoạt HO         |

2. Tham số cho Mobility

eNB: 3 eNB đặt tại các vị trí cố định (200,200), (500,200), (800,200)

UE: bắt đầu tại (100,300) và di chuyển theo vector vận tốc (speed,0,0)

👉 Nếu muốn thử RandomWalkMobility, bạn có thể bỏ comment đoạn:

ueMobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                            "Bounds", RectangleValue(Rectangle(200, 1500, 200, 1500)),
                            "Distance", DoubleValue(500.0),
                            "Speed", StringValue("ns3::UniformRandomVariable[Min=10.0|Max=16.4]"),
                            "Direction", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=6.283185]"));

3. Tham số cho Handover Algorithm

Có 2 loại được hỗ trợ:

🔹 A2A4 RSRQ (comment trong code)
lteHelper->SetHandoverAlgorithmType("ns3::A2A4RsrqHandoverAlgorithm");
lteHelper->SetHandoverAlgorithmAttribute("ServingCellThreshold", UintegerValue(30));
lteHelper->SetHandoverAlgorithmAttribute("NeighbourCellOffset", UintegerValue(1));

🔹 A3 RSRP (đang dùng)
lteHelper->SetHandoverAlgorithmType("ns3::A3RsrpHandoverAlgorithm");
lteHelper->SetHandoverAlgorithmAttribute("Hysteresis", DoubleValue(3.0));
lteHelper->SetHandoverAlgorithmAttribute("TimeToTrigger", TimeValue(MilliSeconds(256)));


Bạn có thể điều chỉnh Hysteresis (1–5 dB) và TimeToTrigger (100–512 ms) để thấy sự khác biệt trong quá trình handover.

4. Tham số cho Fading model

Đang sử dụng mô hình EVA 60 km/h:

lteHelper->SetFadingModel("ns3::TraceFadingLossModel");
lteHelper->SetFadingModelAttribute("TraceFilename", StringValue("src/lte/model/fading-traces/fading_trace_EVA_60kmph.fad"));
lteHelper->SetFadingModelAttribute("TraceLength", TimeValue(Seconds(10)));
lteHelper->SetFadingModelAttribute("SamplesNum", UintegerValue(10000));
lteHelper->SetFadingModelAttribute("WindowSize", TimeValue(Seconds(0.5)));
lteHelper->SetFadingModelAttribute("RbNum", UintegerValue(100));


👉 Bạn có thể thay trace file nếu muốn mô phỏng các điều kiện khác (ví dụ Pedestrian, Vehicular).

Trace file ở đường dẫn sau: /ns-3.45/src/lte/model/fading-traces

5. Kết quả đầu ra

Sau khi chạy mô phỏng, bạn sẽ có:

NetAnim visualization → file lte2.xml để mở trong NetAnim (có node UE, eNB với icon riêng).

Throughput vs Time → Gnuplot xuất ra:
<pre> gnuplot TimeVSThroughput.plt </pre>

TimeVSThroughput.png

SINR và RSRP → Gnuplot xuất ra:
<pre> gnuplot plotRSRP.txt </pre>
<pre> gnuplot plotSINR.txt </pre>

time_vs_rsrp.png
time_vs_sinr.png

FlowMonitor stats → ThroughputMonitor.xml

PCAP trace → file lena-x2-handover-measures-*.pcap

6. Chạy mô phỏng

Trong thư mục ns-3.45/ chạy:
<pre> ./ns3 run scratch/Lena-x2-handover-measures.cc </pre>
