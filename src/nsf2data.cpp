// ported to gcc by alekhin0w.
// original code by Shiru

#define _CRT_SECURE_NO_WARNINGS	//to allow fopen etc be used in VC2008 Express without warnings

#include <stdlib.h>
#include <stdio.h>
#include <string.h>



#define OUT_NESASM	0
#define OUT_CA65	1
#define OUT_ASM6	2

char db[6];
char dw[6];
char local;

FILE *file;
int size;

unsigned char memory[65536];
bool log;
bool change;
int wait;
int duration;

int regs[32];
int effectlen;



static inline unsigned char mem_rd(int adr)
{
	if(adr<0x2000) return memory[adr&0x7ff];

	return memory[adr];
}



static inline void mem_wr(int adr,unsigned char data)
{
	const unsigned char regmap[32]={0,255,1,2,3,255,4,5,6,255,7,8,9,255,10};

	if(adr<0x2000)
	{
		memory[adr&0x7ff]=data;
		return;
	}
	if(adr>=0x5c00&&adr<0x8000)
	{
		memory[adr]=data;
		return;
	}
	if(adr<0x4018)
	{
		if(!log) return;
		if(adr>=0x4000&&adr<0x4020)
		{
			if(adr==0x4001||adr==0x4005||adr==0x400d||adr==0x400f||adr==0x4011||adr==0x4015||adr==0x4017) return; //ignore sweep, noise length counter, DAC, channels enable/disable, interrupt
			if(regs[adr-0x4000]!=data)
			{
				if(!change)
				{
					while(wait>238)
					{
						fprintf(file,"\t%s $%2.2x\n",db,237+0x10);
						effectlen++;
						size++;
						wait-=237;
					}
					if(wait>=0)
					{
						fprintf(file,"\t%s $%2.2x\n",db,wait+0x10);
						effectlen++;
						size++;
					}
					wait=0;
				}

				regs[adr-0x4000]=data;
				fprintf(file,"\t%s $%2.2x,$%2.2x\n",db,regmap[adr-0x4000],data);
				effectlen+=2;
				size+=2;
				change=true;
			}
			return;
		}
	}
}



#include "cpu2a03.h"



int main(int argc,char *argv[])
{
	char name[1024];
	int i,j,cnt;
	unsigned char *nsf;
	int songs,load,init,play;
	int outtype;

	if(argc<2)
	{
		printf("nsf2data converter for FamiTone audio library\n");
		printf("by Shiru (shiru@mail.ru) 06'11\n");
		printf("Usage: nsf2data.exe filename.nsf [-ca65 or -asm6]\n");
		return 1;
	}

	outtype=OUT_NESASM;

	if(argc>2)
	{
		if(!strcasecmp(argv[2],"-ca65")) outtype=OUT_CA65;
		if(!strcasecmp(argv[2],"-asm6")) outtype=OUT_ASM6;
	}

	switch(outtype)
	{
	case OUT_NESASM:
		printf("Output format: NESASM\n");
		strcpy(db,".db");
		strcpy(dw,".dw");
		local='.';
		break;
	case OUT_CA65:
		printf("Output format: CA65\n");
		strcpy(db,".byte");
		strcpy(dw,".word");
		local='@';
		break;
	case OUT_ASM6:
		printf("Output format: Asm6\n");
		strcpy(db,"db");
		strcpy(dw,"dw");
		local='@';
		break;
	}

	file=fopen(argv[1],"rb");

	if(!file)
	{
		printf("Can't open file\n");
		return 1;
	}

	fseek(file,0,SEEK_END);
	size=ftell(file);
	fseek(file,0,SEEK_SET);
	nsf=(unsigned char*)malloc(size);
	fread(nsf,size,1,file);
	fclose(file);

	songs=nsf[0x06];
	load=nsf[0x08]+(nsf[0x09]<<8);
	init=nsf[0x0a]+(nsf[0x0b]<<8);
	play=nsf[0x0c]+(nsf[0x0d]<<8);

	for(i=0x70;i<0x78;i++)
	{
		if(nsf[i])
		{
			printf("Bankswitching is not supported\n");
			free(nsf);
			return 1;
		}
	}

	if(nsf[0x7b])
	{
		printf("Extra sound chips are not supported\n");
		free(nsf);
		return 1;
	}

	printf("%i effects\n",songs);

	memset(memory,0,65536);
	memcpy(memory+load,nsf+0x80,size-0x80);

	free(nsf);

	strcpy(name,argv[1]);
	if(outtype!=OUT_CA65)
	{
		name[strlen(name)-3]='a';
		name[strlen(name)-2]='s';
		name[strlen(name)-1]='m';
	}
	else
	{
		name[strlen(name)-3]='s';
		name[strlen(name)-2]=0;
	}

	file=fopen(name,"wt");

	size=songs*2;

	fprintf(file,"sounds:\n");

	for(i=0;i<songs;i++) fprintf(file,"\t%s %csfx%i\n",dw,local,i);

	fprintf(file,"\n");

	for(j=0;j<songs;j++)
	{
		fprintf(file,"%csfx%i:\n",local,j);

		for(i=0;i<32;i++) regs[i]=-1;
		regs[0x00]=0x30;
		regs[0x04]=0x30;
		regs[0x08]=0x00;
		regs[0x0c]=0x30;

		cpu_reset();
		CPU.A=j;
		CPU.X=0;
		CPU.PC.hl=init;
		log=false;

		for(i=0;i<1000;i++) cpu_tick(); //1000 is enough for FT init

		cpu_reset();
		log=true;
		effectlen=0;
		cnt=0;
		wait=-1;
		duration=0;

		while(true)
		{
			CPU.PC.hl=play;
			CPU.jam=false;
			change=false;

			for(i=0;i<30000/4;i++)
			{
				cpu_tick();
				if(CPU.jam) break;
			}

			if(!change) wait++;
			duration++;
			if(wait>5*60) break;
			if(duration>10*60)
			{
				printf("Effect %i is longer than 10 seconds, forgot Cxx to mark end of the effect?\n",j);
				break;
			}
		}

		fprintf(file,"\t%s $ff\n",db);
		effectlen++;
		size++;

		if(effectlen<256)
		{
			printf("Effect %i size is %i bytes\n",j,effectlen);
		}
		else
		{
			printf("Effect %i is %i bytes, it is too long!\n",j,effectlen);
		}
	}

	fclose(file);

	printf("Binary data size %i\n",size);

	return 0;
}
