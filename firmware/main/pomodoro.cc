#include "pomodoro.h"

int Pomodoro::DurationOf(Phase p) {
    switch (p) {
        case kWork:
            return kWorkMin * 60;
        case kShortBreak:
            return kShortMin * 60;
        case kLongBreak:
            return kLongMin * 60;
    }
    return kWorkMin * 60;
}

// Phase kế khi phase hiện tại KẾT THÚC tự nhiên (hết giờ).
Pomodoro::Phase Pomodoro::NextPhase() const {
    if (phase_ == kWork) {
        // completed_work_ đã được +1 trước khi gọi -> cứ đủ 4 phiên thì nghỉ dài.
        return (completed_work_ % kSessionsBeforeLong == 0) ? kLongBreak : kShortBreak;
    }
    return kWork;  // nghỉ xong -> làm việc
}

void Pomodoro::EnterPhase(Phase p) {
    phase_ = p;
    remaining_s_ = DurationOf(p);
}

void Pomodoro::Reset() {
    completed_work_ = 0;
    state_ = kIdle;
    EnterPhase(kWork);
}

void Pomodoro::Toggle() {
    state_ = (state_ == kRunning) ? kPaused : kRunning;
}

void Pomodoro::Skip() {
    // Bỏ qua: sang phase kế nhưng KHÔNG tính là hoàn thành (work bị skip không +1).
    Phase next = (phase_ == kWork)
                     ? ((completed_work_ % kSessionsBeforeLong == kSessionsBeforeLong - 1) ? kLongBreak
                                                                                           : kShortBreak)
                     : kWork;
    EnterPhase(next);
    state_ = kPaused;
}

void Pomodoro::Tick(int seconds) {
    if (state_ != kRunning) return;
    remaining_s_ -= seconds;
    if (remaining_s_ > 0) return;

    // Hết giờ phase hiện tại -> chuyển phase, giữ trạng thái running (tự chạy tiếp).
    if (phase_ == kWork) completed_work_++;  // chỉ tính khi hoàn thành tự nhiên
    EnterPhase(NextPhase());
}
