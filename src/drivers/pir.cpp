#include "drv_pir.h"
#include "../config/pins.h"

void pir_init() {
    //pinMode(PIN_PIR, INPUT);
    PIR_DDR &= ~(1 << PIR_BIT);
    PIR_PORT &= ~(1 << PIR_BIT);
}

bool pir_read(float *out) {
    //*out = (digitalRead(PIN_PIR) == HIGH) ? 1.0f : 0.0f;
    *out = ((PIR_PINR >> PIR_BIT) & 1) == HIGH ? 1.0f : 0.0f;
    
    return true;
}
