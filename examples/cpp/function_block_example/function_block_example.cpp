/**
 * Adds a reference device that outputs sine waves. Its noisy output signal is averagedSine
 * by an statistics function block. Both the noisy and averaged sine waves are rendered with
 * the renderer function block for 10s.
 */

#include <chrono>
#include <thread>
#include <opendaq/opendaq.h>

using namespace daq;

int main(int /*argc*/, const char* /*argv*/[])
{
    // Create an Instance, loading modules at MODULE_PATH
    const InstancePtr instance = Instance(MODULE_PATH);

    // Add a reference device and set it as root
    auto device = instance.addDevice("daqref://device0");

    FunctionBlockPtr renderer;
#if 0
    // Add statistics and scale function block
    FunctionBlockPtr statistics = instance.addFunctionBlock("ref_fb_module_statistics");
    FunctionBlockPtr scaler = instance.addFunctionBlock("ref_fb_module_scaler");
#endif

#if 0
    // Set renderer to draw 2.5s of data
	renderer = instance.addFunctionBlock("ref_fb_module_renderer");
    renderer.setPropertyValue("Duration", 2.5);
#endif

    // Connect user space functionblocks
    daq::FunctionBlockPtr pStat1, pScaler1, pScaler2;
    auto channels = device.getChannels();
    int channelIdx = 0;
    for (auto channel:channels)
    {
        // Search for functionblocks
        auto fbs = channel.getFunctionBlocks();
    	const auto sineChannel = device.getChannels()[0];
    	const auto sineSignal = sineChannel.getSignals()[0];
    	
        for (auto fb : fbs) {
            if (fb.getLocalId() == "Scaler1") {
                pScaler1 = fb;
            } else if (fb.getLocalId() == "Scaler2") {
                pScaler2 = fb;
            } else if (fb.getLocalId() == "Stat1") {
                pStat1 = fb;
            }
        }

        // Connect functionblocks
        pScaler1.getInputPorts()[0].connect(sineSignal);
        pScaler2.getInputPorts()[0].connect(pScaler1.getSignals()[0]);
        pStat1.getInputPorts()[0].connect(pScaler2.getSignals()[0]);
        if (renderer.assigned()) {
            if (channelIdx < 4) {
                renderer.getInputPorts()[channelIdx].connect(pStat1.getSignals()[0]);
            }
        }
        channelIdx++;
    }

    //renderer.getInputPorts()[1].connect(sineSignal2);

    // Process and render data for 10s, modulating the amplitude
    double ampl_step = 0.1;
    for (int i = 0; i < 300; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        //const double ampl = sineChannel1.getPropertyValue("Amplitude");
        //if (9.95 < ampl || ampl < 3.05)
        //    ampl_step *= -1;
        //sineChannel1.setPropertyValue("Amplitude", ampl + ampl_step);
    }

    printf("Program ended\n");

    return 0;
}
