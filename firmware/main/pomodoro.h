#ifndef POMODORO_H
#define POMODORO_H

// Logic Pomodoro thuần (không LVGL, không RTOS) — dễ test, tách khỏi phần vẽ.
// Chu kỳ: WORK 25' -> nghỉ ngắn 5' (x3) -> sau 4 phiên WORK thì nghỉ dài 15' -> lặp lại.
class Pomodoro {
public:
    enum Phase { kWork, kShortBreak, kLongBreak };
    enum State { kIdle, kRunning, kPaused };

    void Reset();            // về WORK, idle, đếm về số phiên = 0
    void Toggle();           // idle/paused -> running ; running -> paused
    void Skip();             // bỏ qua phase hiện tại sang phase kế (tạm dừng, không tính hoàn thành)
    void Tick(int seconds);  // chỉ chạy khi running; hết giờ -> tự sang phase kế (giữ trạng thái running)

    State state() const { return state_; }
    Phase phase() const { return phase_; }
    bool running() const { return state_ == kRunning; }
    int remaining() const { return remaining_s_; }  // giây còn lại của phase hiện tại
    int duration() const { return DurationOf(phase_); }
    int completed_work() const { return completed_work_; }
    // Số thứ tự phiên WORK trong chu kỳ 4 phiên (1..4) để hiển thị "Phien n/4".
    int work_index() const { return (completed_work_ % kSessionsBeforeLong) + 1; }
    static int sessions_before_long() { return kSessionsBeforeLong; }

private:
    static constexpr int kWorkMin = 25;
    static constexpr int kShortMin = 5;
    static constexpr int kLongMin = 15;
    static constexpr int kSessionsBeforeLong = 4;

    static int DurationOf(Phase p);
    void EnterPhase(Phase p);
    Phase NextPhase() const;  // phase kế sau khi phase hiện tại kết thúc

    Phase phase_ = kWork;
    State state_ = kIdle;
    int remaining_s_ = kWorkMin * 60;
    int completed_work_ = 0;
};

#endif  // POMODORO_H
