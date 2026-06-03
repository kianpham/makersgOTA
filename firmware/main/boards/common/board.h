#ifndef BOARD_H
#define BOARD_H

// Board nền — rút gọn theo Xiaozhi (bỏ network/led/camera/assets/audio ở Phase 1).
// Singleton qua create_board() (board cụ thể khai DECLARE_BOARD).
#include <string>

#include "backlight.h"

class Display;

void* create_board();

class Board {
public:
    static Board& GetInstance() {
        static Board* instance = static_cast<Board*>(create_board());
        return *instance;
    }

    Board(const Board&) = delete;
    Board& operator=(const Board&) = delete;
    virtual ~Board() = default;

    virtual std::string GetBoardType() = 0;
    virtual std::string GetUuid() { return uuid_; }
    virtual Backlight* GetBacklight() { return nullptr; }
    virtual Display* GetDisplay();
    virtual void StartNetwork() {}  // WifiBoard override: provisioning-or-STA (blocking)

protected:
    Board();
    std::string GenerateUuid();
    std::string uuid_;
};

#define DECLARE_BOARD(BOARD_CLASS_NAME) \
    void* create_board() { return new BOARD_CLASS_NAME(); }

#endif  // BOARD_H
