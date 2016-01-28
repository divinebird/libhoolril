#ifndef TRANSMITTER_H
#define	TRANSMITTER_H

#ifdef	__cplusplus
extern "C" {
#endif

struct TransDataHeader {
    uint8_t funcIdentifier;
    int32_t token;
    int32_t datalen;
};

typedef struct {
    struct TransDataHeader header;
    uint8_t data[];
} TransData;

void putNextData(TransData* data);
uint8_t isTransmittionActive();
int initSocket();

#ifdef	__cplusplus
}
#endif

#endif	/* TRANSMITTER_H */

