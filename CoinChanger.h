#ifndef CoinChanger_h
#define CoinChanger_h

#define address 0x08
#define no_response_time 2000

#define just_reset 0x0B
#define dispense 0x05

int coin_scaling_factor = 0;
int coin_type_values[16];
float credit = 0.0f;

class CoinChanger
{
    public:
        CoinChanger();
        ~CoinChanger();

        bool Create();

        void Enable();
        void Dispense();
        void Poll();

    private:
        void Reset();
        void Setup();
        void Status();
        void Type();
        void Expansion();
}

#endif