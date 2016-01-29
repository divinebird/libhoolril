#ifndef TRANSMITTER_H
#define	TRANSMITTER_H

#ifdef	__cplusplus
extern "C" {
#endif

#pragma pack(push)
#pragma pack(1)

typedef struct {
    uint8_t funcIdentifier;
    int command;
    int32_t token;
    int32_t datalen;
} TransDataHeader;

typedef struct {
    TransDataHeader header;
    uint8_t data[];
} TransData;

#pragma pack(pop)

void putNextData(TransData* data);
uint8_t isTransmittionActive();
int initSocket();

#ifdef	__cplusplus
}
#endif

#endif	/* TRANSMITTER_H */

