/*
 * kernelsymbolconsts.h
 *
 *  Created on: 28.05.2010
 *      Author: chrschn
 */

#ifndef KERNELSYMBOLCONSTS_H_
#define KERNELSYMBOLCONSTS_H_

namespace kSym {
    enum Versions {
        VERSION_MIN = 11,
        VERSION_11 = 11,
        VERSION_12 = 12,
        VERSION_MAX = 12
    };
    static const qint32 fileMagic = 0x4B53594D; // "KSYM"
    static const qint16 fileVersion = VERSION_11;
//    static const qint16 flagCompressed = 1;
}

#endif /* KERNELSYMBOLCONSTS_H_ */
