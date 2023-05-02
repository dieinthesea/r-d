// QTAtom:
//   The base-class for atoms in a QuickTime file.

#ifndef QTAtom_H
#define QTAtom_H

// Includes
#include "OSHeaders.h"

#include "QTFile.h"


class QTAtom {

public:
    // Constructors and destructor.
                        QTAtom(QTFile * File, QTFile::AtomTOCEntry * Atom,
                               Bool16 Debug = false, Bool16 DeepDebug = false);
    virtual             ~QTAtom(void);


    // Initialization functions.
    virtual Bool16      Initialize(void) { return true; }

    static SInt64   NTOH64(SInt64 networkOrdered)
    {
    #if BIGENDIAN
        return networkOrdered;
    #else
        return (SInt64) (  (UInt64)  (networkOrdered << 56) | (UInt64)  (((UInt64) 0x00ff0000 << 32) & (networkOrdered << 40))
            | (UInt64)  ( ((UInt64)  0x0000ff00 << 32) & (networkOrdered << 24)) | (UInt64)  (((UInt64)  0x000000ff << 32) & (networkOrdered << 8))
            | (UInt64)  ( ((UInt64)  0x00ff0000 << 8) & (networkOrdered >> 8)) | (UInt64)     ((UInt64)  0x00ff0000 & (networkOrdered >> 24))
            | (UInt64)  (  (UInt64)  0x0000ff00 & (networkOrdered >> 40)) | (UInt64)  ((UInt64)  0x00ff & (networkOrdered >> 56)) );
    #endif
    }

    // Read functions.
            Bool16      ReadBytes(UInt64 Offset, char * Buffer, UInt32 Length);
            Bool16      ReadInt8(UInt64 Offset, UInt8 * Datum);
            Bool16      ReadInt16(UInt64 Offset, UInt16 * Datum);
            Bool16      ReadInt32(UInt64 Offset, UInt32 * Datum);
            Bool16      ReadInt64(UInt64 Offset, UInt64 * Datum);
            Bool16      ReadInt32To64(UInt64 Offset, UInt64 * Datum);
            Bool16		ReadInt32To64Signed(UInt64 Offset, SInt64 * Datum);
            
            Bool16      ReadSubAtomBytes(const char * AtomPath, char * Buffer, UInt32 Length);
            Bool16      ReadSubAtomInt8(const char * AtomPath, UInt8 * Datum);
            Bool16      ReadSubAtomInt16(const char * AtomPath, UInt16 * Datum);
            Bool16      ReadSubAtomInt32(const char * AtomPath, UInt32 * Datum);
            Bool16      ReadSubAtomInt64(const char * AtomPath, UInt64 * Datum);
            
            char*       MemMap(UInt64 Offset, UInt32 Length);
            Bool16      UnMap(char *memPtr, UInt32 Length);
    
    // Debugging functions.
    virtual void        DumpAtom(void) {}


protected:
    //
    // Protected member variables.
    Bool16              fDebug, fDeepDebug;
    QTFile              *fFile;

    QTFile::AtomTOCEntry fTOCEntry;
};

#endif // QTAtom_H
