/*
 * kernelsymbolconsts.h
 *
 *  Created on: 28.05.2010
 *      Author: chrschn
 */

#ifndef KERNELSYMBOLCONSTS_H_
#define KERNELSYMBOLCONSTS_H_

  /*
   * If another Version is added, adapt new Version in:
   *  * referencetype
   *  * structuredmember
   *  * memspecs
   */

namespace kSym {
    enum Versions {
        VERSION_MIN = 11,
        VERSION_11  = 11,
        VERSION_12  = 12,
        VERSION_13  = 13,
        VERSION_14  = 14,
        VERSION_15  = 15,
        VERSION_16  = 16,
        VERSION_17  = 17,
        VERSION_18  = 18,
        VERSION_19  = 19,
        VERSION_20  = 20,
        VERSION_21  = 21,
        VERSION_22  = 22,
        VERSION_MAX = 22
    };
    static const qint32 fileMagic = 0x4B53594D; // "KSYM"
    static const qint16 fileVersion = VERSION_MAX;
//    static const qint16 flagCompressed = 1;
}

#endif /* KERNELSYMBOLCONSTS_H_ */
