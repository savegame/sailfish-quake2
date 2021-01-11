#include "q3_png.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>

#ifndef INT_MAX
#define INT_MAX 0x1fffffff
#endif

#define S_COLOR_YELLOW // "[Warning]: "
#define Com_Error(type, args...) fprintf(stderr, "[%s]: ", #type);\
	fprintf(stderr, ##args)
#define Com_DPrintf(args...) fprintf(stdout, ##args)
#define Com_Printf(args...) fprintf(stderr, ##args)

#define ri_Free(ptr) free(ptr); \
	ptr = NULL
#define ri_FS_FreeFile(ptr) ri_Free(ptr)
#define ri_Malloc(size) malloc(size)
#define Hunk_AllocateTempMemory(len) calloc(1, len)

#define FS_FCloseFile(h) fclose((FILE *)h)
#define FS_Flush(f) fflush((FILE *)f)
#define FS_Read(buf, len, h) fread(buf, 1, len, h)

#define BigLong(x) LongSwap(x)
#define local static            /* for local function definitions */

#define MAXBITS 15              /* maximum bits in a code */
#define MAXLCODES 286           /* maximum number of literal/length codes */
#define MAXDCODES 30            /* maximum number of distance codes */
#define MAXCODES (MAXLCODES+MAXDCODES)  /* maximum codes lengths to read */
#define FIXLCODES 288           /* number of fixed literal/length codes */

/* PNG */
#define PNG_ColourType_Grey      (0)
#define PNG_ColourType_True      (2)
#define PNG_ColourType_Indexed   (3)
#define PNG_ColourType_GreyAlpha (4)
#define PNG_ColourType_TrueAlpha (6)

#define PNG_InterlaceMethod_NonInterlaced (0)
#define PNG_InterlaceMethod_Interlaced    (1)

#define PNG_FilterMethod_0 (0)
#define PNG_CompressionMethod_0 (0)

#define Q3IMAGE_BYTESPERPIXEL (4)

#define PNG_Signature_Size (8)
#define PNG_Signature "\x89\x50\x4E\x47\xD\xA\x1A\xA"

#define PNG_ZlibHeader_Size (2)
#define PNG_ZlibCheckValue_Size (4)

#define PNG_NumColourComponents_Grey      (1)
#define PNG_NumColourComponents_True      (3)
#define PNG_NumColourComponents_Indexed   (1)
#define PNG_NumColourComponents_GreyAlpha (2)
#define PNG_NumColourComponents_TrueAlpha (4)

#define PNG_BitDepth_1  ( 1)
#define PNG_BitDepth_2  ( 2)
#define PNG_BitDepth_4  ( 4)
#define PNG_BitDepth_8  ( 8)
#define PNG_BitDepth_16 (16)

#define PNG_Adam7_NumPasses (7)

#define PNG_FilterType_None    (0)
#define PNG_FilterType_Sub     (1)
#define PNG_FilterType_Up      (2)
#define PNG_FilterType_Average (3)
#define PNG_FilterType_Paeth   (4)

#define PNG_ChunkHeader_Size (8)
#define PNG_Chunk_IHDR_Size (13)
#define PNG_ChunkCRC_Size (4)

#define MAKE_CHUNKTYPE(a,b,c,d) (((a) << 24) | ((b) << 16) | ((c) << 8) | ((d)))
#define PNG_ChunkType_IHDR MAKE_CHUNKTYPE('I', 'H', 'D', 'R')
#define PNG_ChunkType_PLTE MAKE_CHUNKTYPE('P', 'L', 'T', 'E')
#define PNG_ChunkType_IDAT MAKE_CHUNKTYPE('I', 'D', 'A', 'T')
#define PNG_ChunkType_IEND MAKE_CHUNKTYPE('I', 'E', 'N', 'D')
#define PNG_ChunkType_tRNS MAKE_CHUNKTYPE('t', 'R', 'N', 'S')


typedef unsigned char 		byte;
typedef enum {
	qfalse, qtrue
}	qboolean;
typedef unsigned int uint32_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef short int16_t;
typedef int int32_t;

typedef uint32_t PNG_ChunkCRC;
typedef FILE * fileHandle_t; // C-std IO

struct PNG_Chunk_IHDR
{
	uint32_t Width;
	uint32_t Height;
	uint8_t  BitDepth;
	uint8_t  ColourType;
	uint8_t  CompressionMethod;
	uint8_t  FilterMethod;
	uint8_t  InterlaceMethod;
};

struct BufferedFile
{
	byte *Buffer;
	int   Length;
	byte *Ptr;
	int   BytesLeft;
};

struct PNG_ChunkHeader
{
	uint32_t Length;
	uint32_t Type;
};

struct state {
    /* output state */
    uint8_t *out;         /* output buffer */
    uint32_t outlen;       /* available space at out */
    uint32_t outcnt;       /* bytes written to out so far */

    /* input state */
    uint8_t *in;          /* input buffer */
    uint32_t inlen;        /* available input at in */
    uint32_t incnt;        /* bytes read so far */
    int32_t bitbuf;                 /* bit buffer */
    int32_t bitcnt;                 /* number of bits in bit buffer */

    /* input limit error return state for bits() and decode() */
    jmp_buf env;
};

struct huffman {
    int16_t *count;       /* number of symbols of each length */
    int16_t *symbol;      /* canonically ordered symbols */
};


static int    LongSwap (int l)
{
	byte    b1,b2,b3,b4;

	b1 = l&255;
	b2 = (l>>8)&255;
	b3 = (l>>16)&255;
	b4 = (l>>24)&255;

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

local int32_t bits(struct state *s, int32_t need)
{
    int32_t val;           /* bit accumulator (can use up to 20 bits) */

    /* load at least need bits into val */
    val = s->bitbuf;
    while (s->bitcnt < need) {
        if (s->incnt == s->inlen) longjmp(s->env, 1);   /* out of input */
        val |= (int32_t)(s->in[s->incnt++]) << s->bitcnt;  /* load eight bits */
        s->bitcnt += 8;
    }

    /* drop need bits and update buffer, always zero to seven bits left */
    s->bitbuf = (int32_t)(val >> need);
    s->bitcnt -= need;

    /* return need bits, zeroing the bits above that */
    return (int32_t)(val & ((1L << need) - 1));
}

local int32_t decode(struct state *s, struct huffman *h)
{
    int32_t len;            /* current number of bits in code */
    int32_t code;           /* len bits being decoded */
    int32_t first;          /* first code of length len */
    int32_t count;          /* number of codes of length len */
    int32_t index;          /* index of first code of length len in symbol table */
    int32_t bitbuf;         /* bits from stream */
    int32_t left;           /* bits left in next or left to process */
    int16_t *next;        /* next number of codes */

    bitbuf = s->bitbuf;
    left = s->bitcnt;
    code = first = index = 0;
    len = 1;
    next = h->count + 1;
    while (1) {
        while (left--) {
            code |= bitbuf & 1;
            bitbuf >>= 1;
            count = *next++;
            if (code < first + count) { /* if length len, return symbol */
                s->bitbuf = bitbuf;
                s->bitcnt = (s->bitcnt - len) & 7;
                return h->symbol[index + (code - first)];
            }
            index += count;             /* else update for next length */
            first += count;
            first <<= 1;
            code <<= 1;
            len++;
        }
        left = (MAXBITS+1) - len;
        if (left == 0) break;
        if (s->incnt == s->inlen) longjmp(s->env, 1);   /* out of input */
        bitbuf = s->in[s->incnt++];
        if (left > 8) left = 8;
    }
    return -9;                          /* ran out of codes */
}

local int32_t construct(struct huffman *h, int16_t *length, int32_t n)
{
    int32_t symbol;         /* current symbol when stepping through length[] */
    int32_t len;            /* current length when stepping through h->count[] */
    int32_t left;           /* number of possible codes left of current length */
    int16_t offs[MAXBITS+1];      /* offsets in symbol table for each length */

    /* count number of codes of each length */
    for (len = 0; len <= MAXBITS; len++)
        h->count[len] = 0;
    for (symbol = 0; symbol < n; symbol++)
        (h->count[length[symbol]])++;   /* assumes lengths are within bounds */
    if (h->count[0] == n)               /* no codes! */
        return 0;                       /* complete, but decode() will fail */

    /* check for an over-subscribed or incomplete set of lengths */
    left = 1;                           /* one possible code of zero length */
    for (len = 1; len <= MAXBITS; len++) {
        left <<= 1;                     /* one more bit, double codes left */
        left -= h->count[len];          /* deduct count from possible codes */
        if (left < 0) return left;      /* over-subscribed--return negative */
    }                                   /* left > 0 means incomplete */

    /* generate offsets into symbol table for each length for sorting */
    offs[1] = 0;
    for (len = 1; len < MAXBITS; len++)
        offs[len + 1] = offs[len] + h->count[len];

    /*
     * put symbols in table sorted by length, by symbol order within each
     * length
     */
    for (symbol = 0; symbol < n; symbol++)
        if (length[symbol] != 0)
            h->symbol[offs[length[symbol]]++] = symbol;

    /* return zero for complete set, positive for incomplete set */
    return left;
}

local int32_t codes(struct state *s,
                struct huffman *lencode,
                struct huffman *distcode)
{
    int32_t symbol;         /* decoded symbol */
    int32_t len;            /* length for copy */
    uint32_t dist;          /* distance for copy */
    static const int16_t lens[29] = { /* Size base for length codes 257..285 */
        3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
        35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
    static const int16_t lext[29] = { /* Extra bits for length codes 257..285 */
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
        3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
    static const int16_t dists[30] = { /* Offset base for distance codes 0..29 */
        1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
        257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
        8193, 12289, 16385, 24577};
    static const int16_t dext[30] = { /* Extra bits for distance codes 0..29 */
        0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
        7, 7, 8, 8, 9, 9, 10, 10, 11, 11,
        12, 12, 13, 13};

    /* decode literals and length/distance pairs */
    do {
        symbol = decode(s, lencode);
        if (symbol < 0) return symbol;  /* invalid symbol */
        if (symbol < 256) {             /* literal: symbol is the byte */
            /* write out the literal */
            if (s->out != NULL) {
                if (s->outcnt == s->outlen) return 1;
                s->out[s->outcnt] = symbol;
            }
            s->outcnt++;
        }
        else if (symbol > 256) {        /* length */
            /* get and compute length */
            symbol -= 257;
            if (symbol >= 29) return -9;        /* invalid fixed code */
            len = lens[symbol] + bits(s, lext[symbol]);

            /* get and check distance */
            symbol = decode(s, distcode);
            if (symbol < 0) return symbol;      /* invalid symbol */
            dist = dists[symbol] + bits(s, dext[symbol]);
            if (dist > s->outcnt)
                return -10;     /* distance too far back */

            /* copy length bytes from distance bytes back */
            if (s->out != NULL) {
                if (s->outcnt + len > s->outlen) return 1;
                while (len--) {
                    s->out[s->outcnt] = s->out[s->outcnt - dist];
                    s->outcnt++;
                }
            }
            else
                s->outcnt += len;
        }
    } while (symbol != 256);            /* end of block symbol */

    /* done with a valid fixed or dynamic block */
    return 0;
}

local int32_t stored(struct state *s)
{
    uint32_t len;       /* length of stored block */

    /* discard leftover bits from current byte (assumes s->bitcnt < 8) */
    s->bitbuf = 0;
    s->bitcnt = 0;

    /* get length and check against its one's complement */
    if (s->incnt + 4 > s->inlen) return 2;      /* not enough input */
    len = s->in[s->incnt++];
    len |= s->in[s->incnt++] << 8;
    if (s->in[s->incnt++] != (~len & 0xff) ||
        s->in[s->incnt++] != ((~len >> 8) & 0xff))
        return -2;                              /* didn't match complement! */

    /* copy len bytes from in to out */
    if (s->incnt + len > s->inlen) return 2;    /* not enough input */
    if (s->out != NULL) {
        if (s->outcnt + len > s->outlen)
            return 1;                           /* not enough output space */
        while (len--)
            s->out[s->outcnt++] = s->in[s->incnt++];
    }
    else {                                      /* just scanning */
        s->outcnt += len;
        s->incnt += len;
    }

    /* done with a valid stored block */
    return 0;
}

local int32_t fixed(struct state *s)
{
    static int32_t virgin = 1;
    static int16_t lencnt[MAXBITS+1], lensym[FIXLCODES];
    static int16_t distcnt[MAXBITS+1], distsym[MAXDCODES];
    static struct huffman lencode = {lencnt, lensym};
    static struct huffman distcode = {distcnt, distsym};

    /* build fixed huffman tables if first call (may not be thread safe) */
    if (virgin) {
        int32_t symbol;
        int16_t lengths[FIXLCODES];

        /* literal/length table */
        for (symbol = 0; symbol < 144; symbol++)
            lengths[symbol] = 8;
        for (; symbol < 256; symbol++)
            lengths[symbol] = 9;
        for (; symbol < 280; symbol++)
            lengths[symbol] = 7;
        for (; symbol < FIXLCODES; symbol++)
            lengths[symbol] = 8;
        construct(&lencode, lengths, FIXLCODES);

        /* distance table */
        for (symbol = 0; symbol < MAXDCODES; symbol++)
            lengths[symbol] = 5;
        construct(&distcode, lengths, MAXDCODES);

        /* do this just once */
        virgin = 0;
    }

    /* decode data until end-of-block code */
    return codes(s, &lencode, &distcode);
}

local int32_t dynamic(struct state *s)
{
    int32_t nlen, ndist, ncode;             /* number of lengths in descriptor */
    int32_t index;                          /* index of lengths[] */
    int32_t err;                            /* construct() return value */
    int16_t lengths[MAXCODES];            /* descriptor code lengths */
    int16_t lencnt[MAXBITS+1], lensym[MAXLCODES];         /* lencode memory */
    int16_t distcnt[MAXBITS+1], distsym[MAXDCODES];       /* distcode memory */
    struct huffman lencode = {lencnt, lensym};          /* length code */
    struct huffman distcode = {distcnt, distsym};       /* distance code */
    static const int16_t order[19] =      /* permutation of code length codes */
        {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

    /* get number of lengths in each table, check lengths */
    nlen = bits(s, 5) + 257;
    ndist = bits(s, 5) + 1;
    ncode = bits(s, 4) + 4;
    if (nlen > MAXLCODES || ndist > MAXDCODES)
        return -3;                      /* bad counts */

    /* read code length code lengths (really), missing lengths are zero */
    for (index = 0; index < ncode; index++)
        lengths[order[index]] = bits(s, 3);
    for (; index < 19; index++)
        lengths[order[index]] = 0;

    /* build huffman table for code lengths codes (use lencode temporarily) */
    err = construct(&lencode, lengths, 19);
    if (err != 0) return -4;            /* require complete code set here */

    /* read length/literal and distance code length tables */
    index = 0;
    while (index < nlen + ndist) {
        int32_t symbol;             /* decoded value */
        int32_t len;                /* last length to repeat */

        symbol = decode(s, &lencode);
        if (symbol < 16)                /* length in 0..15 */
            lengths[index++] = symbol;
        else {                          /* repeat instruction */
            len = 0;                    /* assume repeating zeros */
            if (symbol == 16) {         /* repeat last length 3..6 times */
                if (index == 0) return -5;      /* no last length! */
                len = lengths[index - 1];       /* last length */
                symbol = 3 + bits(s, 2);
            }
            else if (symbol == 17)      /* repeat zero 3..10 times */
                symbol = 3 + bits(s, 3);
            else                        /* == 18, repeat zero 11..138 times */
                symbol = 11 + bits(s, 7);
            if (index + symbol > nlen + ndist)
                return -6;              /* too many lengths! */
            while (symbol--)            /* repeat last or zero symbol times */
                lengths[index++] = len;
        }
    }

    /* build huffman table for literal/length codes */
    err = construct(&lencode, lengths, nlen);
    if (err < 0 || (err > 0 && nlen - lencode.count[0] != 1))
        return -7;      /* only allow incomplete codes if just one code */

    /* build huffman table for distance codes */
    err = construct(&distcode, lengths + nlen, ndist);
    if (err < 0 || (err > 0 && ndist - distcode.count[0] != 1))
        return -8;      /* only allow incomplete codes if just one code */

    /* decode data until end-of-block code */
    return codes(s, &lencode, &distcode);
}
static int32_t puff(uint8_t  *dest,           /* pointer to destination pointer */
             uint32_t *destlen,        /* amount of output space */
             uint8_t  *source,         /* pointer to source data pointer */
             uint32_t *sourcelen)      /* amount of input available */
{
    struct state s;             /* input/output state */
    int32_t last, type;             /* block information */
    int32_t err;                    /* return value */

    /* initialize output state */
    s.out = dest;
    s.outlen = *destlen;                /* ignored if dest is NULL */
    s.outcnt = 0;

    /* initialize input state */
    s.in = source;
    s.inlen = *sourcelen;
    s.incnt = 0;
    s.bitbuf = 0;
    s.bitcnt = 0;

    /* return if bits() or decode() tries to read past available input */
    if (setjmp(s.env) != 0)             /* if came back here via longjmp() */
        err = 2;                        /* then skip do-loop, return error */
    else {
        /* process blocks until last block or error */
        do {
            last = bits(&s, 1);         /* one if last block */
            type = bits(&s, 2);         /* block type 0..3 */
            err = type == 0 ? stored(&s) :
                  (type == 1 ? fixed(&s) :
                   (type == 2 ? dynamic(&s) :
                    -1));               /* type == 3, invalid */
            if (err != 0) break;        /* return with error */
        } while (!last);
    }

    /* update the lengths and return */
    if (err <= 0) {
        *destlen = s.outcnt;
        *sourcelen = s.incnt;
    }
    return err;
}

static int FS_FOpenFileRead( const char *filename, void **file, qboolean uniqueFILE ) {
	if(!filename || !file)
		return 0;
	FILE *f = fopen(filename, "rb");
	if(!f)
		return 0;
	fseek(f, 0, SEEK_END);
	unsigned long l = ftell(f);
	if(l == 0)
	{
		fclose(f);
		return 0;
	}
	fseek(f, 0, SEEK_SET);
	if(uniqueFILE)
	{
		*file = NULL;
		*file = malloc(l);
		memset(*file, 0, l);
		fread(*file, 1, l, f);
		fclose(f);
	}
	else
	{
		*file = f;
	}
	return l;
}

static int ri_FS_ReadFile( const char *qpath, void **buffer ) {
	fileHandle_t h;
	byte*			buf;
	int				len;

	if ( !qpath || !qpath[0] ) {
		Com_Error( ERR_FATAL, "FS_ReadFile with empty name\n" );
	}

	buf = NULL;	// quiet compiler warning

	// look for it in the filesystem or pack files
	len = FS_FOpenFileRead( qpath, &h, qfalse );
	if ( len == 0 ) {
		if ( buffer ) {
			*buffer = NULL;
		}
		return -1;
	}
	
	if ( !buffer ) {
		FS_FCloseFile( h);
		return len;
	}

	buf = Hunk_AllocateTempMemory(len+1);
	*buffer = buf;

	FS_Read (buf, len, h);

	// guarantee that it will have a trailing 0 for string operations
	buf[len] = 0;
	FS_FCloseFile( h );

	return len;
}

static qboolean BufferedFileRewind(struct BufferedFile *BF, unsigned Offset)
{
	unsigned BytesRead; 

	/*
	 *  input verification
	 */

	if(!BF)
	{
		return(qfalse);
	}

	/*
	 *  special trick to rewind to the beginning of the buffer
	 */

	if(Offset == (unsigned)-1)
	{
		BF->Ptr       = BF->Buffer;
		BF->BytesLeft = BF->Length;

		return(qtrue);
	}

	/*
	 *  How many bytes do we have already read?
	 */

	BytesRead = BF->Ptr - BF->Buffer;

	/*
	 *  We can only rewind to the beginning of the BufferedFile.
	 */

	if(Offset > BytesRead)
	{
		return(qfalse);
	}

	/*
	 *  lower the pointer and counter.
	 */

	BF->Ptr       -= Offset;
	BF->BytesLeft += Offset;

	return(qtrue);
}

static qboolean BufferedFileSkip(struct BufferedFile *BF, unsigned Offset)
{
	/*
	 *  input verification
	 */

	if(!BF)
	{
		return(qfalse);
	}

	/*
	 *  We can only skip to the end of the BufferedFile.
	 */

	if(Offset > BF->BytesLeft)
	{
		return(qfalse);
	}

	/*
	 *  lower the pointer and counter.
	 */

	BF->Ptr       += Offset;
	BF->BytesLeft -= Offset;

	return(qtrue);
}

static struct BufferedFile *ReadBufferedFile(const char *name)
{
	struct BufferedFile *BF;
	union {
		byte *b;
		void *v;
	} buffer;

	/*
	 *  input verification
	 */

	if(!name)
	{
		return(NULL);
	}

	/*
	 *  Allocate control struct.
	 */

	BF = ri_Malloc(sizeof(struct BufferedFile));
	if(!BF)
	{
		return(NULL);
	}

	/*
	 *  Initialize the structs components.
	 */

	BF->Length    = 0;
	BF->Buffer    = NULL;
	BF->Ptr       = NULL;
	BF->BytesLeft = 0;

	/*
	 *  Read the file.
	 */

	BF->Length = ri_FS_ReadFile((char *) name, &buffer.v);
	BF->Buffer = buffer.b;

	/*
	 *  Did we get it? Is it big enough?
	 */

	if(!(BF->Buffer && (BF->Length > 0)))
	{
		ri_Free(BF);

		return(NULL);
	}

	/*
	 *  Set the pointers and counters.
	 */

	BF->Ptr       = BF->Buffer;
	BF->BytesLeft = BF->Length;

	return(BF);
}

static void CloseBufferedFile(struct BufferedFile *BF)
{
	if(BF)
	{
		if(BF->Buffer)
		{
			ri_FS_FreeFile(BF->Buffer);
		}

		ri_Free(BF);
	}
}

static void *BufferedFileRead(struct BufferedFile *BF, unsigned Length)
{
	void *RetVal;

	/*
	 *  input verification
	 */

	if(!(BF && Length))
	{
		return(NULL);
	}

	/*
	 *  not enough bytes left
	 */

	if(Length > BF->BytesLeft)
	{
		return(NULL);
	}

	/*
	 *  the pointer to the requested data
	 */

	RetVal = BF->Ptr;

	/*
	 *  Raise the pointer and counter.
	 */

	BF->Ptr       += Length;
	BF->BytesLeft -= Length;

	return(RetVal);
}

static qboolean FindChunk(struct BufferedFile *BF, uint32_t ChunkType)
{
	struct PNG_ChunkHeader *CH;

	uint32_t Length;
	uint32_t Type;

	/*
	 *  input verification
	 */

	if(!BF)
	{
		return(qfalse);
	}

	/*
	 *  cycle trough the chunks
	 */

	while(qtrue)
	{
		/*
		 *  Read the chunk-header.
		 */

		CH = BufferedFileRead(BF, PNG_ChunkHeader_Size);
		if(!CH)
		{
			return(qfalse);
		}

		/*
		 *  Do not swap the original types
		 *  they might be needed later.
		 */

		Length = BigLong(CH->Length);
		Type   = BigLong(CH->Type);

		/*
		 *  We found it!
		 */

		if(Type == ChunkType)
		{
			/*
			 *  Rewind to the start of the chunk.
			 */

			BufferedFileRewind(BF, PNG_ChunkHeader_Size);

			break;
		}
		else
		{
			/*
			 *  Skip the rest of the chunk.
			 */

			if(Length)
			{
				if(!BufferedFileSkip(BF, Length + PNG_ChunkCRC_Size))
				{
					return(qfalse);
				}  
			}
		}
	}

	return(qtrue);
}

static uint8_t PredictPaeth(uint8_t a, uint8_t b, uint8_t c)
{
	/*
	 *  a == Left
	 *  b == Up
	 *  c == UpLeft
	 */

	uint8_t Pr;
	int p;
	int pa, pb, pc;

	Pr = 0;

	p  = ((int) a) + ((int) b) - ((int) c);
	pa = abs(p - ((int) a));
	pb = abs(p - ((int) b));
	pc = abs(p - ((int) c));

	if((pa <= pb) && (pa <= pc))
	{
		Pr = a;
	}
	else if(pb <= pc)
	{
		Pr = b;
	}
	else
	{
		Pr = c;
	}

	return(Pr);

}

static qboolean UnfilterImage(uint8_t  *DecompressedData, 
		uint32_t  ImageHeight,
		uint32_t  BytesPerScanline, 
		uint32_t  BytesPerPixel)
{
	uint8_t   *DecompPtr;
	uint8_t   FilterType;
	uint8_t  *PixelLeft, *PixelUp, *PixelUpLeft;
	uint32_t  w, h, p;

	/*
	 *  some zeros for the filters
	 */

	uint8_t Zeros[8] = {0, 0, 0, 0, 0, 0, 0, 0};

	/*
	 *  input verification
	 */

	if(!(DecompressedData && BytesPerPixel))
	{
		return(qfalse);
	}

	/*
	 *  ImageHeight and BytesPerScanline can be zero in small interlaced images.
	 */

	if((!ImageHeight) || (!BytesPerScanline))
	{
		return(qtrue);
	}

	/*
	 *  Set the pointer to the start of the decompressed Data.
	 */

	DecompPtr = DecompressedData;

	/*
	 *  Un-filtering is done in place.
	 */

	/*
	 *  Go trough all scanlines.
	 */

	for(h = 0; h < ImageHeight; h++)
	{
		/*
		 *  Every scanline starts with a FilterType byte.
		 */

		FilterType = *DecompPtr;
		DecompPtr++;

		/*
		 *  Left pixel of the first byte in a scanline is zero.
		 */

		PixelLeft = Zeros;

		/*
		 *  Set PixelUp to previous line only if we are on the second line or above.
		 *
		 *  Plus one byte for the FilterType
		 */

		if(h > 0)
		{
			PixelUp = DecompPtr - (BytesPerScanline + 1);
		}
		else
		{
			PixelUp = Zeros;
		}

		/*
		 * The pixel left to the first pixel of the previous scanline is zero too.
		 */

		PixelUpLeft = Zeros;

		/*
		 *  Cycle trough all pixels of the scanline.
		 */

		for(w = 0; w < (BytesPerScanline / BytesPerPixel); w++)
		{
			/*
			 *  Cycle trough the bytes of the pixel.
			 */

			for(p = 0; p < BytesPerPixel; p++)
			{
				switch(FilterType)
				{ 
					case PNG_FilterType_None :
					{
						/*
						 *  The byte is unfiltered.
						 */

						break;
					}

					case PNG_FilterType_Sub :
					{
						DecompPtr[p] += PixelLeft[p];

						break;
					}

					case PNG_FilterType_Up :
					{
						DecompPtr[p] += PixelUp[p];

						break;
					}

					case PNG_FilterType_Average :
					{
						DecompPtr[p] += ((uint8_t) ((((uint16_t) PixelLeft[p]) + ((uint16_t) PixelUp[p])) / 2));

						break;
					}

					case PNG_FilterType_Paeth :
					{
						DecompPtr[p] += PredictPaeth(PixelLeft[p], PixelUp[p], PixelUpLeft[p]);

						break;
					}

					default :
					{
						return(qfalse);
					}
				}
			}

			PixelLeft = DecompPtr;

			/*
			 *  We only have a upleft pixel if we are on the second line or above.
			 */

			if(h > 0)
			{
				PixelUpLeft = DecompPtr - (BytesPerScanline + 1);
			}

			/*
			 *  Skip to the next pixel.
			 */

			DecompPtr += BytesPerPixel;

			/*
			 *  We only have a previous line if we are on the second line and above.
			 */

			if(h > 0)
			{
				PixelUp = DecompPtr - (BytesPerScanline + 1);
			}
		}
	}

	return(qtrue);
}


static qboolean ConvertPixel(struct PNG_Chunk_IHDR *IHDR,
		byte                  *OutPtr,
		uint8_t               *DecompPtr,
		qboolean               HasTransparentColour,
		uint8_t               *TransparentColour,
		uint8_t               *OutPal)
{
	/*
	 *  input verification
	 */

	if(!(IHDR && OutPtr && DecompPtr && TransparentColour && OutPal))
	{
		return(qfalse);
	}

	switch(IHDR->ColourType)
	{
		case PNG_ColourType_Grey :
		{
			switch(IHDR->BitDepth)
			{
				case PNG_BitDepth_1 :
				case PNG_BitDepth_2 :
				case PNG_BitDepth_4 :
				{
					uint8_t Step;
					uint8_t GreyValue;

					Step = 0xFF / ((1 << IHDR->BitDepth) - 1);

					GreyValue = DecompPtr[0] * Step;

					OutPtr[0] = GreyValue;
					OutPtr[1] = GreyValue;
					OutPtr[2] = GreyValue;
					OutPtr[3] = 0xFF;

					/*
					 *  Grey supports full transparency for one specified colour
					 */

					if(HasTransparentColour)
					{
						if(TransparentColour[1] == DecompPtr[0])
						{
							OutPtr[3] = 0x00;
						}
					}


					break;
				}

				case PNG_BitDepth_8 :
				case PNG_BitDepth_16 :
				{
					OutPtr[0] = DecompPtr[0];
					OutPtr[1] = DecompPtr[0];
					OutPtr[2] = DecompPtr[0];
					OutPtr[3] = 0xFF;

					/*
					 *  Grey supports full transparency for one specified colour
					 */

					if(HasTransparentColour)
					{
						if(IHDR->BitDepth == PNG_BitDepth_8)
						{
							if(TransparentColour[1] == DecompPtr[0])
							{
								OutPtr[3] = 0x00;
							}
						}
						else
						{
							if((TransparentColour[0] == DecompPtr[0]) && (TransparentColour[1] == DecompPtr[1]))
							{
								OutPtr[3] = 0x00;
							}
						}
					}

					break;
				}

				default :
				{
					return(qfalse);
				}
			}

			break;
		}

		case PNG_ColourType_True :
		{
			switch(IHDR->BitDepth)
			{
				case PNG_BitDepth_8 :
				{
					OutPtr[0] = DecompPtr[0];
					OutPtr[1] = DecompPtr[1];
					OutPtr[2] = DecompPtr[2];
					OutPtr[3] = 0xFF;

					/*
					 *  True supports full transparency for one specified colour
					 */

					if(HasTransparentColour)
					{
						if((TransparentColour[1] == DecompPtr[0]) &&
								(TransparentColour[3] == DecompPtr[1]) &&
								(TransparentColour[5] == DecompPtr[2]))
						{
							OutPtr[3] = 0x00;
						}
					}

					break;
				}

				case PNG_BitDepth_16 :
				{
					/*
					 *  We use only the upper byte.
					 */

					OutPtr[0] = DecompPtr[0];
					OutPtr[1] = DecompPtr[2];
					OutPtr[2] = DecompPtr[4];
					OutPtr[3] = 0xFF;

					/*
					 *  True supports full transparency for one specified colour
					 */

					if(HasTransparentColour)
					{
						if((TransparentColour[0] == DecompPtr[0]) && (TransparentColour[1] == DecompPtr[1]) &&
								(TransparentColour[2] == DecompPtr[2]) && (TransparentColour[3] == DecompPtr[3]) &&
								(TransparentColour[4] == DecompPtr[4]) && (TransparentColour[5] == DecompPtr[5]))
						{
							OutPtr[3] = 0x00;
						}
					}

					break;
				}

				default :
				{
					return(qfalse);
				}
			}

			break;
		}

		case PNG_ColourType_Indexed :
		{
			OutPtr[0] = OutPal[DecompPtr[0] * Q3IMAGE_BYTESPERPIXEL + 0];
			OutPtr[1] = OutPal[DecompPtr[0] * Q3IMAGE_BYTESPERPIXEL + 1];
			OutPtr[2] = OutPal[DecompPtr[0] * Q3IMAGE_BYTESPERPIXEL + 2];
			OutPtr[3] = OutPal[DecompPtr[0] * Q3IMAGE_BYTESPERPIXEL + 3];

			break;
		}

		case PNG_ColourType_GreyAlpha :
		{
			switch(IHDR->BitDepth)
			{
				case PNG_BitDepth_8 :
				{
					OutPtr[0] = DecompPtr[0];
					OutPtr[1] = DecompPtr[0];
					OutPtr[2] = DecompPtr[0];
					OutPtr[3] = DecompPtr[1];

					break;
				}

				case PNG_BitDepth_16 :
				{
					/*
					 *  We use only the upper byte.
					 */

					OutPtr[0] = DecompPtr[0];
					OutPtr[1] = DecompPtr[0];
					OutPtr[2] = DecompPtr[0];
					OutPtr[3] = DecompPtr[2];

					break;
				}

				default :
				{
					return(qfalse);
				}
			}

			break;
		}

		case PNG_ColourType_TrueAlpha :
		{
			switch(IHDR->BitDepth)
			{
				case PNG_BitDepth_8 :
				{
					OutPtr[0] = DecompPtr[0];
					OutPtr[1] = DecompPtr[1];
					OutPtr[2] = DecompPtr[2];
					OutPtr[3] = DecompPtr[3];

					break;
				}

				case PNG_BitDepth_16 :
				{
					/*
					 *  We use only the upper byte.
					 */

					OutPtr[0] = DecompPtr[0];
					OutPtr[1] = DecompPtr[2];
					OutPtr[2] = DecompPtr[4];
					OutPtr[3] = DecompPtr[6];

					break;
				}

				default :
				{
					return(qfalse);
				}
			}

			break;
		}

		default :
		{
			return(qfalse);
		}
	}

	return(qtrue);
}

static uint32_t DecompressIDATs(struct BufferedFile *BF, uint8_t **Buffer)
{
	uint8_t  *DecompressedData;
	uint32_t  DecompressedDataLength;

	uint8_t  *CompressedData;
	uint8_t  *CompressedDataPtr;
	uint32_t  CompressedDataLength;

	struct PNG_ChunkHeader *CH;

	uint32_t Length;
	uint32_t Type;

	int BytesToRewind;

	int32_t   puffResult;
	uint8_t  *puffDest;
	uint32_t  puffDestLen;
	uint8_t  *puffSrc;
	uint32_t  puffSrcLen;

	/*
	 *  input verification
	 */

	if(!(BF && Buffer))
	{
		return(-1);
	}

	/*
	 *  some zeroing
	 */

	DecompressedData = NULL;
	DecompressedDataLength = 0;
	*Buffer = DecompressedData;

	CompressedData = NULL;
	CompressedDataLength = 0;

	BytesToRewind = 0;

	/*
	 *  Find the first IDAT chunk.
	 */

	if(!FindChunk(BF, PNG_ChunkType_IDAT))
	{
		return(-1);
	}

	/*
	 *  Count the size of the uncompressed data
	 */

	while(qtrue)
	{
		/*
		 *  Read chunk header
		 */

		CH = BufferedFileRead(BF, PNG_ChunkHeader_Size);
		if(!CH)
		{
			/*
			 *  Rewind to the start of this adventure
			 *  and return unsuccessfull
			 */

			BufferedFileRewind(BF, BytesToRewind);

			return(-1);
		}

		/*
		 *  Length and Type of chunk
		 */

		Length = BigLong(CH->Length);
		Type   = BigLong(CH->Type);

		/*
		 *  We have reached the end of the IDAT chunks
		 */

		if(!(Type == PNG_ChunkType_IDAT))
		{
			BufferedFileRewind(BF, PNG_ChunkHeader_Size); 

			break;
		}

		/*
		 *  Add chunk header to count.
		 */

		BytesToRewind += PNG_ChunkHeader_Size;

		/*
		 *  Skip to next chunk
		 */

		if(Length)
		{
			if(!BufferedFileSkip(BF, Length + PNG_ChunkCRC_Size))
			{
				BufferedFileRewind(BF, BytesToRewind);

				return(-1);
			}

			BytesToRewind += Length + PNG_ChunkCRC_Size;
			CompressedDataLength += Length;
		} 
	}

	BufferedFileRewind(BF, BytesToRewind);

	CompressedData = ri_Malloc(CompressedDataLength);
	if(!CompressedData)
	{
		return(-1);
	}

	CompressedDataPtr = CompressedData;

	/*
	 *  Collect the compressed Data
	 */

	while(qtrue)
	{
		/*
		 *  Read chunk header
		 */

		CH = BufferedFileRead(BF, PNG_ChunkHeader_Size);
		if(!CH)
		{
			ri_Free(CompressedData); 

			return(-1);
		}

		/*
		 *  Length and Type of chunk
		 */

		Length = BigLong(CH->Length);
		Type   = BigLong(CH->Type);

		/*
		 *  We have reached the end of the IDAT chunks
		 */

		if(!(Type == PNG_ChunkType_IDAT))
		{
			BufferedFileRewind(BF, PNG_ChunkHeader_Size); 

			break;
		}

		/*
		 *  Copy the Data
		 */

		if(Length)
		{
			uint8_t *OrigCompressedData;

			OrigCompressedData = BufferedFileRead(BF, Length);
			if(!OrigCompressedData)
			{
				ri_Free(CompressedData); 

				return(-1);
			}

			if(!BufferedFileSkip(BF, PNG_ChunkCRC_Size))
			{
				ri_Free(CompressedData); 

				return(-1);
			}

			memcpy(CompressedDataPtr, OrigCompressedData, Length);
			CompressedDataPtr += Length;
		} 
	}

	/*
	 *  Let puff() calculate the decompressed data length.
	 */

	puffDest    = NULL;
	puffDestLen = 0;

	/*
	 *  The zlib header and checkvalue don't belong to the compressed data.
	 */

	puffSrc    = CompressedData + PNG_ZlibHeader_Size;
	puffSrcLen = CompressedDataLength - PNG_ZlibHeader_Size - PNG_ZlibCheckValue_Size;

	/*
	 *  first puff() to calculate the size of the uncompressed data
	 */

	puffResult = puff(puffDest, &puffDestLen, puffSrc, &puffSrcLen);
	if(!((puffResult == 0) && (puffDestLen > 0)))
	{
		ri_Free(CompressedData);

		return(-1);
	}

	/*
	 *  Allocate the buffer for the uncompressed data.
	 */

	DecompressedData = ri_Malloc(puffDestLen);
	if(!DecompressedData)
	{
		ri_Free(CompressedData);

		return(-1);
	}

	/*
	 *  Set the input again in case something was changed by the last puff() .
	 */

	puffDest   = DecompressedData;
	puffSrc    = CompressedData + PNG_ZlibHeader_Size;
	puffSrcLen = CompressedDataLength - PNG_ZlibHeader_Size - PNG_ZlibCheckValue_Size;

	/*
	 *  decompression puff()
	 */

	puffResult = puff(puffDest, &puffDestLen, puffSrc, &puffSrcLen);

	/*
	 *  The compressed data is not needed anymore.
	 */

	ri_Free(CompressedData);

	/*
	 *  Check if the last puff() was successfull.
	 */

	if(!((puffResult == 0) && (puffDestLen > 0)))
	{
		ri_Free(DecompressedData);

		return(-1);
	}

	/*
	 *  Set the output of this function.
	 */

	DecompressedDataLength = puffDestLen;
	*Buffer = DecompressedData;

	return(DecompressedDataLength);
}

static qboolean DecodeImageNonInterlaced(struct PNG_Chunk_IHDR *IHDR,
		byte                  *OutBuffer, 
		uint8_t               *DecompressedData,
		uint32_t               DecompressedDataLength,
		qboolean               HasTransparentColour,
		uint8_t               *TransparentColour,
		uint8_t               *OutPal)
{
	uint32_t IHDR_Width;
	uint32_t IHDR_Height;
	uint32_t BytesPerScanline, BytesPerPixel, PixelsPerByte;
	uint32_t  w, h, p;
	byte *OutPtr;
	uint8_t *DecompPtr;

	/*
	 *  input verification
	 */

	if(!(IHDR && OutBuffer && DecompressedData && DecompressedDataLength && TransparentColour && OutPal))
	{
		return(qfalse);
	}

	/*
	 *  byte swapping
	 */

	IHDR_Width  = BigLong(IHDR->Width);
	IHDR_Height = BigLong(IHDR->Height);

	/*
	 *  information for un-filtering
	 */

	switch(IHDR->ColourType)
	{
		case PNG_ColourType_Grey :
		{
			switch(IHDR->BitDepth)
			{
				case PNG_BitDepth_1 :
				case PNG_BitDepth_2 :
				case PNG_BitDepth_4 :
				{
					BytesPerPixel    = 1;
					PixelsPerByte    = 8 / IHDR->BitDepth;

					break;
				}

				case PNG_BitDepth_8  :
				case PNG_BitDepth_16 :
				{
					BytesPerPixel    = (IHDR->BitDepth / 8) * PNG_NumColourComponents_Grey;
					PixelsPerByte    = 1;

					break;
				}

				default :
				{
					return(qfalse);
				}
			}

			break;
		}

		case PNG_ColourType_True :
		{
			switch(IHDR->BitDepth)
			{
				case PNG_BitDepth_8  :
				case PNG_BitDepth_16 :
				{
					BytesPerPixel    = (IHDR->BitDepth / 8) * PNG_NumColourComponents_True;
					PixelsPerByte    = 1;

					break;
				}

				default :
				{
					return(qfalse);
				}
			}

			break;
		}

		case PNG_ColourType_Indexed :
		{
			switch(IHDR->BitDepth)
			{
				case PNG_BitDepth_1 :
				case PNG_BitDepth_2 :
				case PNG_BitDepth_4 :
				{
					BytesPerPixel    = 1;
					PixelsPerByte    = 8 / IHDR->BitDepth;

					break;
				}

				case PNG_BitDepth_8 :
				{
					BytesPerPixel    = PNG_NumColourComponents_Indexed;
					PixelsPerByte    = 1;

					break;
				}

				default :
				{
					return(qfalse);
				}
			}

			break;
		}

		case PNG_ColourType_GreyAlpha :
		{
			switch(IHDR->BitDepth)
			{
				case PNG_BitDepth_8 :
				case PNG_BitDepth_16 :
				{
					BytesPerPixel    = (IHDR->BitDepth / 8) * PNG_NumColourComponents_GreyAlpha;
					PixelsPerByte    = 1;

					break;
				}

				default :
				{
					return(qfalse);
				}
			}

			break;
		}

		case PNG_ColourType_TrueAlpha :
		{
			switch(IHDR->BitDepth)
			{
				case PNG_BitDepth_8 :
				case PNG_BitDepth_16 :
				{
					BytesPerPixel    = (IHDR->BitDepth / 8) * PNG_NumColourComponents_TrueAlpha;
					PixelsPerByte    = 1;

					break;
				}

				default :
				{
					return(qfalse);
				}
			}

			break;
		}

		default :
		{
			return(qfalse);
		}
	}

	/*
	 *  Calculate the size of one scanline
	 */

	BytesPerScanline = (IHDR_Width * BytesPerPixel + (PixelsPerByte - 1)) / PixelsPerByte;

	/*
	 *  Check if we have enough data for the whole image.
	 */

	if(!(DecompressedDataLength == ((BytesPerScanline + 1) * IHDR_Height)))
	{
		return(qfalse);
	}

	/*
	 *  Unfilter the image.
	 */

	if(!UnfilterImage(DecompressedData, IHDR_Height, BytesPerScanline, BytesPerPixel))
	{
		return(qfalse);
	}

	/*
	 *  Set the working pointers to the beginning of the buffers.
	 */

	OutPtr = OutBuffer;
	DecompPtr = DecompressedData;

	/*
	 *  Create the output image.
	 */

	for(h = 0; h < IHDR_Height; h++)
	{
		/*
		 *  Count the pixels on the scanline for those multipixel bytes
		 */

		uint32_t CurrPixel;

		/*
		 *  skip FilterType
		 */

		DecompPtr++;

		/*
		 *  Reset the pixel count.
		 */

		CurrPixel = 0;

		for(w = 0; w < (BytesPerScanline / BytesPerPixel); w++)
		{
			if(PixelsPerByte > 1)
			{
				uint8_t  Mask;
				uint32_t Shift;
				uint8_t  SinglePixel;

				for(p = 0; p < PixelsPerByte; p++)
				{
					if(CurrPixel < IHDR_Width)
					{
						Mask  = (1 << IHDR->BitDepth) - 1;
						Shift = (PixelsPerByte - 1 - p) * IHDR->BitDepth;

						SinglePixel = ((DecompPtr[0] & (Mask << Shift)) >> Shift);

						if(!ConvertPixel(IHDR, OutPtr, &SinglePixel, HasTransparentColour, TransparentColour, OutPal))
						{
							return(qfalse);
						}

						OutPtr += Q3IMAGE_BYTESPERPIXEL;
						CurrPixel++;
					}
				}

			}
			else
			{
				if(!ConvertPixel(IHDR, OutPtr, DecompPtr, HasTransparentColour, TransparentColour, OutPal))
				{
					return(qfalse);
				}


				OutPtr += Q3IMAGE_BYTESPERPIXEL;
			}

			DecompPtr += BytesPerPixel;
		}
	}

	return(qtrue);
}

static qboolean DecodeImageInterlaced(struct PNG_Chunk_IHDR *IHDR,
		byte                  *OutBuffer, 
		uint8_t               *DecompressedData,
		uint32_t               DecompressedDataLength,
		qboolean               HasTransparentColour,
		uint8_t               *TransparentColour,
		uint8_t               *OutPal)
{
	uint32_t IHDR_Width;
	uint32_t IHDR_Height;
	uint32_t BytesPerScanline[PNG_Adam7_NumPasses], BytesPerPixel, PixelsPerByte;
	uint32_t PassWidth[PNG_Adam7_NumPasses], PassHeight[PNG_Adam7_NumPasses];
	uint32_t WSkip[PNG_Adam7_NumPasses], WOffset[PNG_Adam7_NumPasses], HSkip[PNG_Adam7_NumPasses], HOffset[PNG_Adam7_NumPasses];
	uint32_t w, h, p, a;
	byte *OutPtr;
	uint8_t *DecompPtr;
	uint32_t TargetLength;

	/*
	 *  input verification
	 */

	if(!(IHDR && OutBuffer && DecompressedData && DecompressedDataLength && TransparentColour && OutPal))
	{
		return(qfalse);
	}

	/*
	 *  byte swapping
	 */

	IHDR_Width  = BigLong(IHDR->Width);
	IHDR_Height = BigLong(IHDR->Height);

	/*
	 *  Skip and Offset for the passes.
	 */

	WSkip[0]   = 8;
	WOffset[0] = 0;
	HSkip[0]   = 8;
	HOffset[0] = 0;

	WSkip[1]   = 8;
	WOffset[1] = 4;
	HSkip[1]   = 8;
	HOffset[1] = 0;

	WSkip[2]   = 4;
	WOffset[2] = 0;
	HSkip[2]   = 8;
	HOffset[2] = 4;

	WSkip[3]   = 4;
	WOffset[3] = 2;
	HSkip[3]   = 4;
	HOffset[3] = 0;

	WSkip[4]   = 2;
	WOffset[4] = 0;
	HSkip[4]   = 4;
	HOffset[4] = 2;

	WSkip[5]   = 2;
	WOffset[5] = 1;
	HSkip[5]   = 2;
	HOffset[5] = 0;

	WSkip[6]   = 1;
	WOffset[6] = 0;
	HSkip[6]   = 2;
	HOffset[6] = 1;

	/*
	 *  Calculate the sizes of the passes.
	 */

	PassWidth[0]  = (IHDR_Width  + 7) / 8;
	PassHeight[0] = (IHDR_Height + 7) / 8;

	PassWidth[1]  = (IHDR_Width  + 3) / 8;
	PassHeight[1] = (IHDR_Height + 7) / 8;

	PassWidth[2]  = (IHDR_Width  + 3) / 4;
	PassHeight[2] = (IHDR_Height + 3) / 8;

	PassWidth[3]  = (IHDR_Width  + 1) / 4;
	PassHeight[3] = (IHDR_Height + 3) / 4;

	PassWidth[4]  = (IHDR_Width  + 1) / 2;
	PassHeight[4] = (IHDR_Height + 1) / 4;

	PassWidth[5]  = (IHDR_Width  + 0) / 2;
	PassHeight[5] = (IHDR_Height + 1) / 2;

	PassWidth[6]  = (IHDR_Width  + 0) / 1;
	PassHeight[6] = (IHDR_Height + 0) / 2;

	/*
	 *  information for un-filtering
	 */

	switch(IHDR->ColourType)
	{
		case PNG_ColourType_Grey :
		{
			switch(IHDR->BitDepth)
			{
				case PNG_BitDepth_1 :
				case PNG_BitDepth_2 :
				case PNG_BitDepth_4 :
				{
					BytesPerPixel    = 1;
					PixelsPerByte    = 8 / IHDR->BitDepth;

					break;
				}

				case PNG_BitDepth_8  :
				case PNG_BitDepth_16 :
				{
					BytesPerPixel    = (IHDR->BitDepth / 8) * PNG_NumColourComponents_Grey;
					PixelsPerByte    = 1;

					break;
				}

				default :
				{
					return(qfalse);
				}
			}

			break;
		}

		case PNG_ColourType_True :
		{
			switch(IHDR->BitDepth)
			{
				case PNG_BitDepth_8  :
				case PNG_BitDepth_16 :
				{
					BytesPerPixel    = (IHDR->BitDepth / 8) * PNG_NumColourComponents_True;
					PixelsPerByte    = 1;

					break;
				}

				default :
				{
					return(qfalse);
				}
			}

			break;
		}

		case PNG_ColourType_Indexed :
		{
			switch(IHDR->BitDepth)
			{
				case PNG_BitDepth_1 :
				case PNG_BitDepth_2 :
				case PNG_BitDepth_4 :
				{
					BytesPerPixel    = 1;
					PixelsPerByte    = 8 / IHDR->BitDepth;

					break;
				}

				case PNG_BitDepth_8 :
				{
					BytesPerPixel    = PNG_NumColourComponents_Indexed;
					PixelsPerByte    = 1;

					break;
				}

				default :
				{
					return(qfalse);
				}
			}

			break;
		}

		case PNG_ColourType_GreyAlpha :
		{
			switch(IHDR->BitDepth)
			{
				case PNG_BitDepth_8 :
				case PNG_BitDepth_16 :
				{
					BytesPerPixel    = (IHDR->BitDepth / 8) * PNG_NumColourComponents_GreyAlpha;
					PixelsPerByte    = 1;

					break;
				}

				default :
				{
					return(qfalse);
				}
			}

			break;
		}

		case PNG_ColourType_TrueAlpha :
		{
			switch(IHDR->BitDepth)
			{
				case PNG_BitDepth_8 :
				case PNG_BitDepth_16 :
				{
					BytesPerPixel    = (IHDR->BitDepth / 8) * PNG_NumColourComponents_TrueAlpha;
					PixelsPerByte    = 1;

					break;
				}

				default :
				{
					return(qfalse);
				}
			}

			break;
		}

		default :
		{
			return(qfalse);
		}
	}

	/*
	 *  Calculate the size of the scanlines per pass
	 */

	for(a = 0; a < PNG_Adam7_NumPasses; a++)
	{
		BytesPerScanline[a] = (PassWidth[a] * BytesPerPixel + (PixelsPerByte - 1)) / PixelsPerByte;
	}

	/*
	 *  Calculate the size of all passes
	 */

	TargetLength = 0;

	for(a = 0; a < PNG_Adam7_NumPasses; a++)
	{
		TargetLength += ((BytesPerScanline[a] + (BytesPerScanline[a] ? 1 : 0)) * PassHeight[a]);
	}

	/*
	 *  Check if we have enough data for the whole image.
	 */

	if(!(DecompressedDataLength == TargetLength))
	{
		return(qfalse);
	}

	/*
	 *  Unfilter the image.
	 */

	DecompPtr = DecompressedData;

	for(a = 0; a < PNG_Adam7_NumPasses; a++)
	{
		if(!UnfilterImage(DecompPtr, PassHeight[a], BytesPerScanline[a], BytesPerPixel))
		{
			return(qfalse);
		}

		DecompPtr += ((BytesPerScanline[a] + (BytesPerScanline[a] ? 1 : 0)) * PassHeight[a]);
	}

	/*
	 *  Set the working pointers to the beginning of the buffers.
	 */

	DecompPtr = DecompressedData;

	/*
	 *  Create the output image.
	 */

	for(a = 0; a < PNG_Adam7_NumPasses; a++)
	{
		for(h = 0; h < PassHeight[a]; h++)
		{
			/*
			 *  Count the pixels on the scanline for those multipixel bytes
			 */

			uint32_t CurrPixel;

			/*
			 *  skip FilterType
			 *  but only when the pass has a width bigger than zero
			 */

			if(BytesPerScanline[a])
			{
				DecompPtr++;
			}

			/*
			 *  Reset the pixel count.
			 */

			CurrPixel = 0;

			for(w = 0; w < (BytesPerScanline[a] / BytesPerPixel); w++)
			{
				if(PixelsPerByte > 1)
				{
					uint8_t  Mask;
					uint32_t Shift;
					uint8_t  SinglePixel;

					for(p = 0; p < PixelsPerByte; p++)
					{
						if(CurrPixel < PassWidth[a])
						{
							Mask  = (1 << IHDR->BitDepth) - 1;
							Shift = (PixelsPerByte - 1 - p) * IHDR->BitDepth;

							SinglePixel = ((DecompPtr[0] & (Mask << Shift)) >> Shift);

							OutPtr = OutBuffer + (((((h * HSkip[a]) + HOffset[a]) * IHDR_Width) + ((CurrPixel * WSkip[a]) + WOffset[a])) * Q3IMAGE_BYTESPERPIXEL);

							if(!ConvertPixel(IHDR, OutPtr, &SinglePixel, HasTransparentColour, TransparentColour, OutPal))
							{
								return(qfalse);
							}

							CurrPixel++;
						}
					}

				}
				else
				{
					OutPtr = OutBuffer + (((((h * HSkip[a]) + HOffset[a]) * IHDR_Width) + ((w * WSkip[a]) + WOffset[a])) * Q3IMAGE_BYTESPERPIXEL);

					if(!ConvertPixel(IHDR, OutPtr, DecompPtr, HasTransparentColour, TransparentColour, OutPal))
					{
						return(qfalse);
					}
				}

				DecompPtr += BytesPerPixel;
			}
		}
	}

	return(qtrue);
}

void vkb_LoadPNG(const char *name, unsigned char **pic, int *width, int *height)
{
	struct BufferedFile *ThePNG;
	byte *OutBuffer;
	uint8_t *Signature;
	struct PNG_ChunkHeader *CH;
	uint32_t ChunkHeaderLength;
	uint32_t ChunkHeaderType;
	struct PNG_Chunk_IHDR *IHDR;
	uint32_t IHDR_Width;
	uint32_t IHDR_Height;
	PNG_ChunkCRC *CRC;
	uint8_t *InPal;
	uint8_t *DecompressedData;
	uint32_t DecompressedDataLength;
	uint32_t i;

	/*
	 *  palette with 256 RGBA entries
	 */

	uint8_t OutPal[1024];

	/*
	 *  transparent colour from the tRNS chunk
	 */

	qboolean HasTransparentColour = qfalse;
	uint8_t TransparentColour[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

	/*
	 *  input verification
	 */

	if(!(name && pic))
	{
		return;
	}

	/*
	 *  Zero out return values.
	 */

	*pic = NULL;

	if(width)
	{
		*width = 0;
	}

	if(height)
	{
		*height = 0;
	}

	/*
	 *  Read the file.
	 */

	ThePNG = ReadBufferedFile(name);
	if(!ThePNG)
	{
		return;
	}           

	/*
	 *  Read the siganture of the file.
	 */

	Signature = BufferedFileRead(ThePNG, PNG_Signature_Size);
	if(!Signature)
	{
		CloseBufferedFile(ThePNG);

		return;
	}

	/*
	 *  Is it a PNG?
	 */

	if(memcmp(Signature, PNG_Signature, PNG_Signature_Size))
	{
		CloseBufferedFile(ThePNG);

		return; 
	}

	/*
	 *  Read the first chunk-header.
	 */

	CH = BufferedFileRead(ThePNG, PNG_ChunkHeader_Size);
	if(!CH)
	{
		CloseBufferedFile(ThePNG);

		return; 
	}

	/*
	 *  PNG multi-byte types are in Big Endian
	 */

	ChunkHeaderLength = BigLong(CH->Length);
	ChunkHeaderType   = BigLong(CH->Type);

	/*
	 *  Check if the first chunk is an IHDR.
	 */

	if(!((ChunkHeaderType == PNG_ChunkType_IHDR) && (ChunkHeaderLength == PNG_Chunk_IHDR_Size)))
	{
		CloseBufferedFile(ThePNG);

		return; 
	}

	/*
	 *  Read the IHDR.
	 */ 

	IHDR = BufferedFileRead(ThePNG, PNG_Chunk_IHDR_Size);
	if(!IHDR)
	{
		CloseBufferedFile(ThePNG);

		return; 
	}

	/*
	 *  Read the CRC for IHDR
	 */

	CRC = BufferedFileRead(ThePNG, PNG_ChunkCRC_Size);
	if(!CRC)
	{
		CloseBufferedFile(ThePNG);

		return; 
	}

	/*
	 *  Here we could check the CRC if we wanted to.
	 */

	/*
	 *  multi-byte type swapping
	 */

	IHDR_Width  = BigLong(IHDR->Width);
	IHDR_Height = BigLong(IHDR->Height);

	/*
	 *  Check if Width and Height are valid.
	 */

	if(!((IHDR_Width > 0) && (IHDR_Height > 0))
	|| IHDR_Width > INT_MAX / Q3IMAGE_BYTESPERPIXEL / IHDR_Height)
	{
		CloseBufferedFile(ThePNG);

		Com_Printf(S_COLOR_YELLOW "%s: invalid image size\n", name);

		return; 
	}

	/*
	 *  Do we need to check if the dimensions of the image are valid for Quake3?
	 */

	/*
	 *  Check if CompressionMethod and FilterMethod are valid.
	 */

	if(!((IHDR->CompressionMethod == PNG_CompressionMethod_0) && (IHDR->FilterMethod == PNG_FilterMethod_0)))
	{
		CloseBufferedFile(ThePNG);

		return; 
	}

	/*
	 *  Check if InterlaceMethod is valid.
	 */

	if(!((IHDR->InterlaceMethod == PNG_InterlaceMethod_NonInterlaced)  || (IHDR->InterlaceMethod == PNG_InterlaceMethod_Interlaced)))
	{
		CloseBufferedFile(ThePNG);

		return;
	}

	/*
	 *  Read palette for an indexed image.
	 */

	if(IHDR->ColourType == PNG_ColourType_Indexed)
	{
		/*
		 *  We need the palette first.
		 */

		if(!FindChunk(ThePNG, PNG_ChunkType_PLTE))
		{
			CloseBufferedFile(ThePNG);

			return;
		}

		/*
		 *  Read the chunk-header.
		 */

		CH = BufferedFileRead(ThePNG, PNG_ChunkHeader_Size);
		if(!CH)
		{
			CloseBufferedFile(ThePNG);

			return; 
		}

		/*
		 *  PNG multi-byte types are in Big Endian
		 */

		ChunkHeaderLength = BigLong(CH->Length);
		ChunkHeaderType   = BigLong(CH->Type);

		/*
		 *  Check if the chunk is an PLTE.
		 */

		if(!(ChunkHeaderType == PNG_ChunkType_PLTE))
		{
			CloseBufferedFile(ThePNG);

			return; 
		}

		/*
		 *  Check if Length is divisible by 3
		 */

		if(ChunkHeaderLength % 3)
		{
			CloseBufferedFile(ThePNG);

			return;   
		}

		/*
		 *  Read the raw palette data
		 */

		InPal = BufferedFileRead(ThePNG, ChunkHeaderLength);
		if(!InPal)
		{
			CloseBufferedFile(ThePNG);

			return; 
		}

		/*
		 *  Read the CRC for the palette
		 */

		CRC = BufferedFileRead(ThePNG, PNG_ChunkCRC_Size);
		if(!CRC)
		{
			CloseBufferedFile(ThePNG);

			return; 
		}

		/*
		 *  Set some default values.
		 */

		for(i = 0; i < 256; i++)
		{
			OutPal[i * Q3IMAGE_BYTESPERPIXEL + 0] = 0x00;
			OutPal[i * Q3IMAGE_BYTESPERPIXEL + 1] = 0x00;
			OutPal[i * Q3IMAGE_BYTESPERPIXEL + 2] = 0x00;
			OutPal[i * Q3IMAGE_BYTESPERPIXEL + 3] = 0xFF;  
		}

		/*
		 *  Convert to the Quake3 RGBA-format.
		 */

		for(i = 0; i < (ChunkHeaderLength / 3); i++)
		{
			OutPal[i * Q3IMAGE_BYTESPERPIXEL + 0] = InPal[i*3+0];
			OutPal[i * Q3IMAGE_BYTESPERPIXEL + 1] = InPal[i*3+1];
			OutPal[i * Q3IMAGE_BYTESPERPIXEL + 2] = InPal[i*3+2];
			OutPal[i * Q3IMAGE_BYTESPERPIXEL + 3] = 0xFF;
		}
	}

	/*
	 *  transparency information is sometimes stored in an tRNS chunk
	 */

	/*
	 *  Let's see if there is a tRNS chunk
	 */

	if(FindChunk(ThePNG, PNG_ChunkType_tRNS))
	{
		uint8_t *Trans;

		/*
		 *  Read the chunk-header.
		 */

		CH = BufferedFileRead(ThePNG, PNG_ChunkHeader_Size);
		if(!CH)
		{
			CloseBufferedFile(ThePNG);

			return; 
		}

		/*
		 *  PNG multi-byte types are in Big Endian
		 */

		ChunkHeaderLength = BigLong(CH->Length);
		ChunkHeaderType   = BigLong(CH->Type);

		/*
		 *  Check if the chunk is an tRNS.
		 */

		if(!(ChunkHeaderType == PNG_ChunkType_tRNS))
		{
			CloseBufferedFile(ThePNG);

			return; 
		}

		/*
		 *  Read the transparency information.
		 */

		Trans = BufferedFileRead(ThePNG, ChunkHeaderLength);
		if(!Trans)
		{
			CloseBufferedFile(ThePNG);

			return;  
		}

		/*
		 *  Read the CRC.
		 */

		CRC = BufferedFileRead(ThePNG, PNG_ChunkCRC_Size);
		if(!CRC)
		{
			CloseBufferedFile(ThePNG);

			return; 
		}

		/*
		 *  Only for Grey, True and Indexed ColourType should tRNS exist.
		 */

		switch(IHDR->ColourType)
		{
			case PNG_ColourType_Grey :
			{
				if(!ChunkHeaderLength == 2)
				{
					CloseBufferedFile(ThePNG);

					return;    
				}

				HasTransparentColour = qtrue;

				/*
				 *  Grey can have one colour which is completely transparent.
				 *  This colour is always stored in 16 bits.
				 */

				TransparentColour[0] = Trans[0];
				TransparentColour[1] = Trans[1];

				break;
			}

			case PNG_ColourType_True :
			{
				if(!ChunkHeaderLength == 6)
				{
					CloseBufferedFile(ThePNG);

					return;    
				}

				HasTransparentColour = qtrue;

				/*
				 *  True can have one colour which is completely transparent.
				 *  This colour is always stored in 16 bits.
				 */

				TransparentColour[0] = Trans[0];
				TransparentColour[1] = Trans[1];
				TransparentColour[2] = Trans[2];
				TransparentColour[3] = Trans[3];
				TransparentColour[4] = Trans[4];
				TransparentColour[5] = Trans[5];

				break;
			}

			case PNG_ColourType_Indexed :
			{
				/*
				 *  Maximum of 256 one byte transparency entries.
				 */

				if(ChunkHeaderLength > 256)
				{
					CloseBufferedFile(ThePNG);

					return;    
				}

				HasTransparentColour = qtrue;

				/*
				 *  alpha values for palette entries
				 */

				for(i = 0; i < ChunkHeaderLength; i++)
				{
					OutPal[i * Q3IMAGE_BYTESPERPIXEL + 3] = Trans[i];
				}

				break;
			}

			/*
			 *  All other ColourTypes should not have tRNS chunks
			 */

			default :
			{
				CloseBufferedFile(ThePNG);

				return;
			}
		} 
	}

	/*
	 *  Rewind to the start of the file.
	 */

	if(!BufferedFileRewind(ThePNG, -1))
	{
		CloseBufferedFile(ThePNG);

		return; 
	}

	/*
	 *  Skip the signature
	 */

	if(!BufferedFileSkip(ThePNG, PNG_Signature_Size))
	{
		CloseBufferedFile(ThePNG);

		return; 
	}

	/*
	 *  Decompress all IDAT chunks
	 */

	DecompressedDataLength = DecompressIDATs(ThePNG, &DecompressedData);
	if(!(DecompressedDataLength && DecompressedData))
	{
		CloseBufferedFile(ThePNG);

		return;
	}

	/*
	 *  Allocate output buffer.
	 */

	OutBuffer = ri_Malloc(IHDR_Width * IHDR_Height * Q3IMAGE_BYTESPERPIXEL); 
	if(!OutBuffer)
	{
		ri_Free(DecompressedData); 
		CloseBufferedFile(ThePNG);

		return;  
	}

	/*
	 *  Interlaced and Non-interlaced images need to be handled differently.
	 */

	switch(IHDR->InterlaceMethod)
	{
		case PNG_InterlaceMethod_NonInterlaced :
		{
			if(!DecodeImageNonInterlaced(IHDR, OutBuffer, DecompressedData, DecompressedDataLength, HasTransparentColour, TransparentColour, OutPal))
			{
				ri_Free(OutBuffer); 
				ri_Free(DecompressedData); 
				CloseBufferedFile(ThePNG);

				return;
			}

			break;
		}

		case PNG_InterlaceMethod_Interlaced :
		{
			if(!DecodeImageInterlaced(IHDR, OutBuffer, DecompressedData, DecompressedDataLength, HasTransparentColour, TransparentColour, OutPal))
			{
				ri_Free(OutBuffer); 
				ri_Free(DecompressedData); 
				CloseBufferedFile(ThePNG);

				return;
			}

			break;
		}

		default :
		{
			ri_Free(OutBuffer); 
			ri_Free(DecompressedData); 
			CloseBufferedFile(ThePNG);

			return;
		}
	}

	/*
	 *  update the pointer to the image data
	 */

	*pic = OutBuffer;

	/*
	 *  Fill width and height.
	 */

	if(width)
	{
		*width = IHDR_Width;
	}

	if(height)
	{
		*height = IHDR_Height;
	}

	/*
	 *  DecompressedData is not needed anymore.
	 */

	ri_Free(DecompressedData); 

	/*
	 *  We have all data, so close the file.
	 */

	CloseBufferedFile(ThePNG);
}

/*
int main(int argc, char *argv[])
{
	byte *data;
	int w;
	int h;
	R_LoadPNG(argv[1], &data, &w, &h);
	printf("%s %p %d %d\n", argv[1], data, w, h);
	free(data);
	return 0;
}
*/
