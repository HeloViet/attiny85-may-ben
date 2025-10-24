#include "Arduino.h"

// ##FF  define

// #define UNO
// #define TINNY85
//=============================================//
#ifdef UNO
#define STEP_PIN 3        // 0      // PB0 - STEP
#define BUTTON_RUN 4      // 1    // PB4 - Nút run
#define BUTTON_TIMER 5    // 2  // PB3 - Nút set timer
#define BUTTON_RUN_1S 6   // 3 // PB3
#define BUTTON_LED 7      // 4    // PB4
#define BUTTON_EN_MOTOR 7 // 4    // PB4
#define pstring(var) Serial.println(F("     " #var));

#else
//  Định nghĩa chân
#define STEP_PIN PB0        // PB0 - STEP
#define BUTTON_RUN PB1      // PB4 - Nút run
#define BUTTON_TIMER PB2    // PB3 - Nút set timer
#define BUTTON_RUN_1S PB3   // PB3
#define BUTTON_LED PB4      // PB4
#define BUTTON_EN_MOTOR PB4 // PB4
// #define pstring(var) ((void)0)

#endif
//=============================================//

#define DEBOUNCE 3000       // Ngưỡng lọc nhiễu
#define SINGLE_TOUCH 130000 // Ngưỡng tối đa cho nhấn ngắn
#define LONG_TOUCH 140000   // Ngưỡng tối thiểu cho nhấn lâu

#define LED_BLINK 150
#define BUTTON_PUSH 100

#define MOTOR_DELAY 2
#define RESET_TIMER 3000

// ##!FF

//=============================================//
// ##FF Variable
uint8_t timerSeconds = 1;      // Thời gian hẹn (giây)
unsigned long reset_timer = 0; // thời gian nhấn nút thì đếm lại từ đầu

typedef enum
{
    Bit_TIMER = 0,
    Bit_RUN,
    Bit_RUN_1S
} button_bit_t;

typedef union
{
    uint8_t raw; // Giá trị thô 16 bit
    struct
    {
        uint8_t single_touch : 1; //
        uint8_t long_touch : 1;   //

        uint8_t reserved : 6;
    };
} button_record_t;

button_record_t timer_button;
button_record_t run_button;
button_record_t run1s_button;

bool busy_state = 0; // ko kích hoạt nút bấm nếu đang có trạng thái thực hiện

uint8_t motor_delay = 10; // 500 us
// ##!FF

//=============================================//
// ##FF function

// Hàm chạy motor trong t giây
void runMotor(uint8_t seconds)
{
    // pstring("=== run motor ===");
    unsigned long startTime = millis();
    digitalWrite(BUTTON_LED, LOW);
    delay(100);
    uint16_t _motor_delay = motor_delay * 200;
    while (millis() - startTime < seconds * 1000)
    {

        // digitalWrite(STEP_PIN, HIGH);
        // delay(MOTOR_DELAY);
        // digitalWrite(STEP_PIN, LOW);
        // delay(MOTOR_DELAY);

        // test motor delay
        digitalWrite(STEP_PIN, HIGH);
        delayMicroseconds(_motor_delay);
        digitalWrite(STEP_PIN, LOW);
        delayMicroseconds(_motor_delay);
    }
    digitalWrite(BUTTON_LED, HIGH);
}

void led_indicate(int _delay, int blink = 1)
{
    for (int i = 0; i < blink; i++)
    {
        digitalWrite(BUTTON_LED, LOW);
        delay(_delay);
        digitalWrite(BUTTON_LED, HIGH);
        delay(_delay);
    }
}

void check_button(uint8_t pin, button_record_t *button, void (*button_func)(void), bool _led_indi = 1)
{
    if (!digitalRead(pin) and !busy_state)
    {
        // for (int i = 0; i < pin; i++)
        // {
        //     led_indicate(50);
        // }
        // Serial.print(pin);
        // pstring(" button_push");
        unsigned long countTouchLong = 0;

        // Chờ button thả hoặc vượt ngưỡng LONG_TOUCH
        while (!digitalRead(pin))
        {
            countTouchLong++;
            if (countTouchLong >= LONG_TOUCH)
            {
                break;
            }
        }
        if (countTouchLong <= DEBOUNCE)
        {
            // Nhấn quá ngắn, bỏ qua (debounce)
            return;
        }

        // Nhấn hợp lệ, xóa cờ single_touch và long_touch
        button->single_touch = 0;
        button->long_touch = 0;

        // Phân loại kiểu nhấn
        if (countTouchLong < SINGLE_TOUCH)
        {
            // Nhấn ngắn, đặt cờ tương ứng với nút và cho hàm button logic chạy
            button->single_touch = 1;
            if (_led_indi)
            {
                led_indicate(BUTTON_PUSH);
                delay(150);
            }
        }
        else if (countTouchLong >= LONG_TOUCH)
        {
            button->long_touch = 1;
            led_indicate(50);
            delay(150); // thời gian nhả nút ko triger single
        }

        busy_state = 1;
        button_func();
        busy_state = 0;
    }
}
// ##!FF

// ##FF button_funtion
void button_timer_func()
{
    // nếu nhấn liên tục thì sẽ tăng timerSeconds, sau thời gian lâu ko nhấn mà nhấn lại
    // thì sẽ đếm lại từ đầu
    if (timer_button.single_touch)
    {
        timer_button.single_touch = 0;
        if (millis() - reset_timer < RESET_TIMER)
            timerSeconds++;

        else
        {
            timerSeconds = 1;
        }

        led_indicate(LED_BLINK, timerSeconds);
        reset_timer = millis();
    }

    // long touch
    else
    {
        timer_button.long_touch = 0;
        motor_delay = motor_delay % 10 + 1; // giá trị chẵn từ 1-10 (200us tới 2000)
        if (motor_delay < 5)
            motor_delay = 5;

        // 4 <= motor_delay<=10; do giá trị 123 motor ko chạy
        led_indicate(LED_BLINK, motor_delay - 4); // 5-10 tương ứng với 1-6 lần nháy
    }
}

void button_run_func()
{
    // pstring(" button_run_func");
    //  button_log.RUN_single_touch = 0;
    runMotor(timerSeconds);
}

void button_run_1s_func()
{
    // pstring(" button_run_1s_func");
    // button_log.RUN_1S_single_touch = 0;
    runMotor(1);
}
// ##!FF
//=============================================//
//  ##FF Setup
void setup()
{
    delay(1000);

#ifdef UNO
    Serial.begin(9600);
#endif

    // pstring("=== Hệ thống sẵn sàng ===");

    pinMode(STEP_PIN, OUTPUT);

    pinMode(BUTTON_TIMER, INPUT_PULLUP);
    pinMode(BUTTON_RUN, INPUT_PULLUP);
    pinMode(BUTTON_RUN_1S, INPUT_PULLUP);

    pinMode(BUTTON_LED, OUTPUT);
    //=============================================//

    digitalWrite(BUTTON_LED, HIGH);
}
// ##!FF Setup

//=============================================//
void loop()
{
    check_button(BUTTON_TIMER, &timer_button, button_timer_func, 0);
    check_button(BUTTON_RUN, &run_button, button_run_func);
    check_button(BUTTON_RUN_1S, &run1s_button, button_run_1s_func);

    // check_button(BUTTON_TIMER, Bit_TIMER, button_timer_func, 0);
    // check_button(BUTTON_RUN, Bit_RUN, button_timer_func);
    // check_button(BUTTON_RUN_1S, Bit_RUN_1S, button_timer_func);

    // runMotor(3);
    // delay(5000);
}