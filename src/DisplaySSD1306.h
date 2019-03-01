#include <SSD1306Ascii.h>
#include <SSD1306AsciiWire.h>

namespace AW {

class DisplaySSD1306 : public TActor {
public:
    void SetContrast(uint8_t contrast) {
        if (Display) {
            Display->setContrast(contrast);
        }
    }

    void DisplayOff() {
        if (Display) {
            Display->ssd1306WriteCmd(SSD1306_DISPLAYOFF);
        }
    }

    void DisplayOn() {
        if (Display) {
            Display->ssd1306WriteCmd(SSD1306_DISPLAYON);
        }
    }

protected:
    SSD1306AsciiWire* Display = nullptr;

    void OnEvent(TEventPtr event, const TActorContext& context) override {
        switch (event->EventID) {
        case TEventBootstrap::EventID:
            return OnBootstrap(static_cast<TEventBootstrap*>(event.Release()), context);
        case TEventData::EventID:
            return OnData(static_cast<TEventData*>(event.Release()), context);
        }
    }

    void OnBootstrap(TUniquePtr<TEventBootstrap>, const TActorContext&/* context*/) {
        static const uint8_t address = 0x3c;
        TWire::BeginTransmission(address);
        if (TWire::EndTransmission()) {
			Display = new SSD1306AsciiWire();
            Display->begin(&Adafruit128x64, address);
            Display->setFont(Adafruit5x7);
            Display->setScroll(true);
            Display->clear();
		}
    }

    void OnData(TUniquePtr<TEventData> event, const TActorContext&/* context*/) {
        if (Display) {
            for (unsigned int i = 0; i < event->Data.size(); ++i) {
                Display->write(event->Data[i]);
            }
            Display->println();
        }
    }
};

}
