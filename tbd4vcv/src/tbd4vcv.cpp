/***************
CTAG TBD >>to be determined<< is an open source eurorack synthesizer module.

A project conceived within the Creative Technologies Arbeitsgruppe of
Kiel University of Applied Sciences: https://www.creative-technologies.de

(c) 2020 by Robert Manzke. All rights reserved.

The CTAG TBD software is licensed under the GNU General Public License
(GPL 3.0), available here: https://www.gnu.org/licenses/gpl-3.0.txt

The CTAG TBD hardware design is released under the Creative Commons
Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0).
Details here: https://creativecommons.org/licenses/by-nc-sa/4.0/

CTAG TBD is provided "as is" without any express or implied warranties.

License and copyright details for specific submodules are included in their
respective component folders / files if different from this license.
***************/

#include "plugin.hpp"
#include "WebServer.hpp"
#include "SPManager.hpp"
#include "esp_spi_flash.h"
#include <iostream>


struct tbd4vcv : rack::Module {
	enum ParamIds {
		BTN_TRIG_0_PARAM,
		BTN_TRIG_1_PARAM,
		POT0_PARAM,
		POT1_PARAM,
		GAIN0_PARAM,
		GAIN1_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		IN0_INPUT,
		IN1_INPUT,
		TRIG0_INPUT,
		TRIG1_INPUT,
        CV0_INPUT,
		CV1_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT0_OUTPUT,
		OUT1_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
        ENUMS(RGB_LIGHT, 3),
        ENUMS(WIFI_LIGHT, 3),
		NUM_LIGHTS
	};

	tbd4vcv() {
        spManager.Start(rack::asset::plugin(pluginInstance, "spiffs_image/"));
        if(instanceCount == 0){
            string fn = rack::asset::plugin(pluginInstance, "sample_rom/sample-rom.tbd");
            spi_flash_emu_init(fn.c_str());
            server.Start(3000, rack::asset::plugin(pluginInstance, "spiffs_image/www"));
            activeServerInstance = this;
            server.SetCurrentSPManager(&this->spManager);
        }
        instanceCount++;
        //rack::logger::log(rack::Level::DEBUG_LEVEL, "tbd4vcv.cpp", 48, "Constructor called");
        //std::cerr << "Instance number " << instanceCount << std::endl;
        //std::cerr << rack::asset::plugin(pluginInstance, "spiffs_image/data/spm-config.jsn") << std::endl;
        //std::cerr << rack::asset::system(rack::asset::plugin(pluginInstance, "spiffs_image/data/spm-config.jsn")) << std::endl;
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(BTN_TRIG_0_PARAM, 0.f, 1.f, 0.f, "");
		configParam(BTN_TRIG_1_PARAM, 0.f, 1.f, 0.f, "");
		configParam(POT0_PARAM, 0.f, 1.f, 0.f, "");
		configParam(POT1_PARAM, 0.f, 1.f, 0.f, "");
		configParam(GAIN0_PARAM, 0.f, 1.f, 0.f, "");
		configParam(GAIN1_PARAM, 0.f, 1.f, 0.f, "");

        chMeters[0].mode = rack::dsp::VuMeter2::Mode::PEAK;
        chMeters[1].mode = rack::dsp::VuMeter2::Mode::PEAK;
	}
    ~tbd4vcv(){
        //rack::logger::log(rack::Level::DEBUG_LEVEL, "tbd4vcv.cpp", 48, "Destructor called");
        instanceCount--;
        if(activeServerInstance == this){
            activeServerInstance = nullptr;
            server.SetCurrentSPManager(nullptr);
        }
        if(instanceCount == 0){
            spi_flash_emu_release();
            server.Stop();
        }
    }

    // instance members
    rack::dsp::SampleRateConverter<2> inputSrc;
    rack::dsp::SampleRateConverter<2> outputSrc;
    rack::dsp::DoubleRingBuffer<rack::dsp::Frame<2>, 256> inputBuffer;
    rack::dsp::DoubleRingBuffer<rack::dsp::Frame<2>, 256> outputBuffer;
    rack::dsp::VuMeter2 chMeters[2];
    int blueDecay {0};
    CTAG::AUDIO::SPManager spManager;
    float wifiStatus {0.3f};

	void process(const ProcessArgs& args) override {
        // Get input
        rack::dsp::Frame<2> inputFrame = {};
        if (!inputBuffer.full()) {
            inputFrame.samples[0] = inputs[IN0_INPUT].getVoltage() / 5.0;
            inputFrame.samples[1] = inputs[IN1_INPUT].getVoltage() / 5.0;
            inputBuffer.push(inputFrame);
        }

        // Render frames
        if (outputBuffer.empty()) {
            float audiobuffer[32*2];
            float cvdata[N_CVS];
            uint8_t trigdata[N_TRIGS];
            // Convert input buffer
            {
                inputSrc.setRates(args.sampleRate, 44100);
                rack::dsp::Frame<2> inputFrames[32];
                int inLen = inputBuffer.size();
                int outLen = 32;
                inputSrc.process(inputBuffer.startData(), &inLen, inputFrames, &outLen);
                inputBuffer.startIncr(inLen);

                // We might not fill all of the input buffer if there is a deficiency, but this cannot be avoided due to imprecisions between the input and output SRC.
                for (int i = 0; i < outLen; i++) {
                    audiobuffer[i*2] = inputFrames[i].samples[0] * params[GAIN0_PARAM].getValue();
                    audiobuffer[i*2 + 1] = inputFrames[i].samples[1] * params[GAIN1_PARAM].getValue();
                }
            }

            cvdata[0] = inputs[CV0_INPUT].getVoltage() / 5.0;
            cvdata[1] = inputs[CV1_INPUT].getVoltage() / 5.0;
            cvdata[2] = params[POT0_PARAM].getValue();
            cvdata[3] = params[POT1_PARAM].getValue();


            // inverted logic here
            trigdata[0] = (params[BTN_TRIG_0_PARAM].getValue() > 0.5 ? 1 : 0) || (inputs[TRIG0_INPUT].getVoltage() > 2.5 ? 1 : 0) == 1 ? 0 : 1;
            trigdata[1] = (params[BTN_TRIG_1_PARAM].getValue() > 0.5 ? 1 : 0) || (inputs[TRIG1_INPUT].getVoltage() > 2.5 ? 1 : 0) == 1 ? 0 : 1;

            CTAG::SP::ProcessData pd;
            pd.buf = audiobuffer;
            pd.cv = cvdata;
            pd.trig = trigdata;

            spManager.Process(pd);

            // Convert output buffer
            {
                rack::dsp::Frame<2> outputFrames[32];
                for (int i = 0; i < 32; i++) {
                    outputFrames[i].samples[0] = audiobuffer[i*2];
                    outputFrames[i].samples[1] = audiobuffer[i*2 + 1];
                }

                outputSrc.setRates(44100, args.sampleRate);
                int inLen = 32;
                int outLen = outputBuffer.capacity();
                outputSrc.process(outputFrames, &inLen, outputBuffer.endData(), &outLen);
                outputBuffer.endIncr(outLen);
            }
        }

        // Set output
        rack::dsp::Frame<2> outputFrame = {};
        if (!outputBuffer.empty()) {
            outputFrame = outputBuffer.shift();
            outputs[OUT0_OUTPUT].setVoltage(5.0 * outputFrame.samples[0]);
            outputs[OUT1_OUTPUT].setVoltage(5.0 * outputFrame.samples[1]);
        }

        // wifi led
        if(activeServerInstance == this){
            if(wifiStatus > 0.7f) wifiStatus = 0.3f;
            lights[WIFI_LIGHT + 0].setBrightness(wifiStatus);
            lights[WIFI_LIGHT + 1].setBrightness(wifiStatus);
            lights[WIFI_LIGHT + 2].setBrightness(wifiStatus);
            wifiStatus += args.sampleTime * 0.25f;
        }else{
            lights[WIFI_LIGHT + 0].setBrightness(0.f);
            lights[WIFI_LIGHT + 1].setBrightness(0.f);
            lights[WIFI_LIGHT + 2].setBrightness(0.f);
        }
        // status led
        if(spManager.GetBlueStatus()){
            blueDecay = args.sampleRate / 2.;
        }
        if(blueDecay){
            blueDecay--;
            lights[RGB_LIGHT + 0].setBrightness(0.);
            lights[RGB_LIGHT + 1].setBrightness(0.);
            lights[RGB_LIGHT + 2].setBrightness(1.);
        }else{
            lights[RGB_LIGHT + 2].setBrightness(0.);
            float sum = inputs[IN0_INPUT].getVoltage() / 5.f + inputs[IN1_INPUT].getVoltage() / 5.0;
            chMeters[0].process(args.sampleTime, sum);
            float g = chMeters[0].getBrightness(-30.f, 0.f);
            sum = outputs[OUT0_OUTPUT].getVoltage() / 5.f + outputs[OUT1_OUTPUT].getVoltage() / 5.0;
            chMeters[1].process(args.sampleTime, sum);
            float r = chMeters[1].getBrightness(-30.f, 0.f);
            lights[RGB_LIGHT + 0].setBrightness(r);
            lights[RGB_LIGHT + 1].setBrightness(g);
        }
	}

    json_t* dataToJson() override {
        //rack::logger::log(rack::Level::DEBUG_LEVEL, "tbd4vcv.cpp", 48, "dataToJson called");
        json_t* rootJ = json_loads(spManager.GetSPManagerDataModel().c_str(), 0, NULL);
        return rootJ;
    }

    void dataFromJson(json_t *root) override{
        char * s = json_dumps(root, 0);
//        rack::logger::log(rack::Level::DEBUG_LEVEL, "tbd4vcv.cpp", 48, "dataFromJson called\n%s", s);
        spManager.SetSPManagerDataModel(string(s));
        free(s);
    }

    // static members
    static tbd4vcv* activeServerInstance;
    static WebServer server;
    static int instanceCount;
};

int tbd4vcv::instanceCount {0};
tbd4vcv* tbd4vcv::activeServerInstance {nullptr};
WebServer tbd4vcv::server;

struct tbd4vcvWidget : rack::ModuleWidget {
	tbd4vcvWidget(tbd4vcv* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(rack::asset::plugin(pluginInstance, "res/tbd4vcv.svg")));

		addChild(rack::createWidget<rack::ScrewSilver>(rack::Vec(rack::RACK_GRID_WIDTH, 0)));
		addChild(rack::createWidget<rack::ScrewSilver>(rack::Vec(box.size.x - 2 * rack::RACK_GRID_WIDTH, 0)));
		addChild(rack::createWidget<rack::ScrewSilver>(rack::Vec(rack::RACK_GRID_WIDTH, rack::RACK_GRID_HEIGHT - rack::RACK_GRID_WIDTH)));
		addChild(rack::createWidget<rack::ScrewSilver>(rack::Vec(box.size.x - 2 * rack::RACK_GRID_WIDTH, rack::RACK_GRID_HEIGHT - rack::RACK_GRID_WIDTH)));

		addParam(rack::createParamCentered<rack::BefacoPush>(rack::mm2px(rack::Vec(7.406, 49.539)), module, tbd4vcv::BTN_TRIG_0_PARAM));
		addParam(rack::createParamCentered<rack::BefacoPush>(rack::mm2px(rack::Vec(32.806, 49.539)), module, tbd4vcv::BTN_TRIG_1_PARAM));
		addParam(rack::createParamCentered<rack::Trimpot>(rack::mm2px(rack::Vec(7.406, 62.333)), module, tbd4vcv::POT0_PARAM));
		addParam(rack::createParamCentered<rack::Trimpot>(rack::mm2px(rack::Vec(32.806, 62.333)), module, tbd4vcv::POT1_PARAM));
		addParam(rack::createParamCentered<rack::Trimpot>(rack::mm2px(rack::Vec(7.406, 78.933)), module, tbd4vcv::GAIN0_PARAM));
		addParam(rack::createParamCentered<rack::Trimpot>(rack::mm2px(rack::Vec(32.806, 78.933)), module, tbd4vcv::GAIN1_PARAM));

		addInput(rack::createInputCentered<rack::PJ301MPort>(rack::mm2px(rack::Vec(4.897, 96.524)), module, tbd4vcv::IN0_INPUT));
		addInput(rack::createInputCentered<rack::PJ301MPort>(rack::mm2px(rack::Vec(15.057, 96.524)), module, tbd4vcv::IN1_INPUT));
		addInput(rack::createInputCentered<rack::PJ301MPort>(rack::mm2px(rack::Vec(25.217, 96.524)), module, tbd4vcv::TRIG0_INPUT));
		addInput(rack::createInputCentered<rack::PJ301MPort>(rack::mm2px(rack::Vec(35.377, 96.524)), module, tbd4vcv::TRIG1_INPUT));
		addInput(rack::createInputCentered<rack::PJ301MPort>(rack::mm2px(rack::Vec(25.217, 109.478)), module, tbd4vcv::CV0_INPUT));
		addInput(rack::createInputCentered<rack::PJ301MPort>(rack::mm2px(rack::Vec(35.377, 109.478)), module, tbd4vcv::CV1_INPUT));

		addOutput(rack::createOutputCentered<rack::PJ301MPort>(rack::mm2px(rack::Vec(4.897, 109.478)), module, tbd4vcv::OUT0_OUTPUT));
		addOutput(rack::createOutputCentered<rack::PJ301MPort>(rack::mm2px(rack::Vec(15.057, 109.478)), module, tbd4vcv::OUT1_OUTPUT));

        addChild(rack::createLightCentered<rack::LargeLight<rack::RedGreenBlueLight>>(rack::mm2px(rack::Vec(20.123, 57.802)), module, tbd4vcv::RGB_LIGHT));
        addChild(rack::createLightCentered<rack::SmallLight<rack::RedGreenBlueLight>>(rack::mm2px(rack::Vec(4.187+3.25, 9.742+3.25)), module, tbd4vcv::WIFI_LIGHT));
		//addChild(rack::createLightCentered<rack::MediumLight<rack::RedLight>>(rack::mm2px(rack::Vec(20.123, 57.802)), module, tbd4vcv::BTN_TRIG_0_LIGHT));
	}

    void appendContextMenu(rack::Menu* menu) override {
        //rack::logger::log(rack::Level::DEBUG_LEVEL, "tbd4vcv.cpp", 98, "appendContextMenu called");
        tbd4vcv* module = dynamic_cast<tbd4vcv*>(this->module);

        menu->addChild(new rack::MenuEntry);
        menu->addChild(rack::createMenuLabel("Enable Web Server"));

        // happens when action is performed on view
        struct ServerItem : rack::MenuItem {
            tbd4vcv* module;
            void onAction(const rack::event::Action& e) override {
                if(module->activeServerInstance != module){
                    // TODO: maybe there is a mutex needed here, as the web server accesses this from a different thread!
                    module->activeServerInstance = module;
                    module->server.SetCurrentSPManager(&module->spManager);
                }
            }
        };

        // happens when view is rendered
        std::string serverItemName = {"Active"};
        ServerItem* serverItem = rack::createMenuItem<ServerItem>(serverItemName);
        serverItem->rightText = CHECKMARK(module == module->activeServerInstance);
        serverItem->module = module;
        menu->addChild(serverItem);
    }
};


rack::Model* modeltbd4vcv = rack::createModel<tbd4vcv, tbd4vcvWidget>("tbd4vcv");