#include "cuppa.hpp"

#include <random>

namespace cuppa
{
    extern std::unique_ptr<platformDriverInterface> createPlatformDriverMSWindows();
    extern std::unique_ptr<graphicsDriverInterface> createGraphicsDriverGdiplus();
    extern void assertionError(const char*);

    void Assert(bool _condition, const char* _message)
    {
        if(!_condition)
        {
            assertionError( _message );
            exit(1);
            return;
        }
    }

    void app::run()
    {
        // Commandline: __argc, __argv

        platformDriver = createPlatformDriverMSWindows();

        graphicsDriver = createGraphicsDriverGdiplus();
        graphicsDriver->setup();

        setup();
        platformDriver->setup(*this);
        platformDriver->run();
    }

    std::default_random_engine engine{ std::random_device()() };
    std::uniform_real_distribution<float> uniform_distribution{ 0.0f, 1.0f };
    std::normal_distribution<float> normal_distribution{ };

    float app::random() const { return uniform_distribution(engine); }
    float app::randomGaussian() const { return normal_distribution(engine); }
} // namespace cuppa