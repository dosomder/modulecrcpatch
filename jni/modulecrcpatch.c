/*
 * modulecrcpatch by zxz0O0
 *
 * reads a symbol crc from input file and patches the crc in output file
 * another way would be to create a kernel module for reading crc from kallsyms
 * but usually it's only required to patch module_layout, so this is easier
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//symbols *should* be in the first 4096 bytes, maybe it's more if there are many
#define filebufsize 4096
#define isascii(c)  ((c & ~0x7F) == 0)
//#define DEBUG

typedef struct {
	char crc32[4];
	char symbolname[60];
} KOSymbol;

void debugprintf(const char* format, ...)
{
#ifdef DEBUG
	va_list argptr;
    va_start(argptr, format);
	vfprintf(stdout, format, argptr);
	va_end(argptr);
#endif
}

int memsearch(char* buf, int buflength, const char* pattern, int patternlength)
{
	int i;
	debugprintf("searching for: 0x%08x\n", *(unsigned int*)pattern);

	for(i = 0; i < (buflength - patternlength); i++)
	{
		if(i == (buflength - (patternlength + 1))) //+1 for 0x00
		{
			debugprintf("error: memcmp didn't find anything\n");
			return 0;
		}

		if(memcmp(pattern, &buf[i], patternlength) != 0)
			continue;

		return i;
	}
}

int main(int argc, char** argv)
{
	int retvalue = 0;
	printf("\nmodulecrcpatch (by zxz0O0)\n\n");
	if(argc != 3)
	{
		printf("Usage: modulecrcpatch [readfile] [writefile]\n");
		printf("Reads symbol crc from readfile and patches crc in writefile\n");
		return 0;
	}

	char* arg_readfile = argv[1];
	char* arg_writefile = argv[2];

	FILE* readfile = fopen(arg_readfile, "rb");
	if(readfile==NULL)
	{
		printf("Error: [readfile] %s does not exist\n", arg_readfile);
		return 1;
	}

	FILE* writefile = fopen(arg_writefile, "rb+");
	if(writefile==NULL)
	{
		printf("Error: [writefile] %s does not exist\n", arg_writefile);
		return 1;
	}

	char* readbuf = malloc(filebufsize);
	char* writebuf = malloc(filebufsize);
	if(readbuf==NULL || writebuf==NULL)
	{
		printf("Error allocating memory\n");
		return 1;
	}

	memset(readbuf, 0, filebufsize);
	fread(readbuf, filebufsize, 1, readfile);
	fclose(readfile);

	memset(writebuf, 0, filebufsize);
	int bread = fread(writebuf, filebufsize, 1, writefile);

	debugprintf("first 4 read 0x%08x\n", *(unsigned int*)readbuf);
	debugprintf("first 4 write 0x%08x\n", *(unsigned int*)writebuf);

	const char mlstr[] = "module_layout\0\0\0"; //it's char [60] but we don't really care
	const char gnustr[] = "GNU";
	const int gnupos = memsearch(writebuf, filebufsize, gnustr, sizeof(gnustr)) - 0x168; //module name is saved in this char mname[0x168];
	//const since we use it as pointer for kosymbols
	const int mlpos = memsearch(writebuf, filebufsize, mlstr, sizeof(mlstr)) - 4; //size of crc
	if(gnupos <= 0 || mlpos <= 0)
	{
		printf("Error finding pos\n");
		retvalue = 1;
		goto _return;
	}

	//get length of all KO Symbols
	int kosymbolsize = (gnupos - mlpos);
	//at the end is some additional stuff we are not sure about the length so we round it down
	kosymbolsize = kosymbolsize - (kosymbolsize % sizeof(KOSymbol));
	//now divide the length through length of KOSymbol struct to get the number of KO Symbols
	kosymbolsize = kosymbolsize / sizeof(KOSymbol);
	//there should be atleast module_layout
	if(kosymbolsize < 1)
	{
		printf("Error kosymbolsize\n");
		retvalue = 1;
		goto _return;
	}

	KOSymbol* kosymbols = (KOSymbol*)&writebuf[mlpos];
	int i;
	for(i = 0;i < kosymbolsize; i++)
	{
		printf("%s: ", kosymbols[i].symbolname);
		int inputpos = memsearch(readbuf, filebufsize, kosymbols[i].symbolname, sizeof(kosymbols[i].symbolname)) - 4; //-4 to go to start of crc
		if(inputpos < 0)
		{
			printf("not found\n");
			continue;
		}

		if(memcmp(kosymbols[i].crc32, &readbuf[inputpos], 4) == 0)
		{
			printf("match\n");
			continue;
		}

		memcpy(kosymbols[i].crc32, &readbuf[inputpos], 4);
		printf("patched to 0x%08X\n", *(unsigned int*)&readbuf[inputpos]);
	}

	fseek(writefile, 0L, SEEK_SET);
	fwrite(writebuf, filebufsize, 1, writefile);

_return:
	free(readbuf);
	free(writebuf);
	fclose(writefile);
	return retvalue;
}