#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <math.h>

#define HW_REGS_BASE 0xFF200000 //I/O mapped base 주소
#define HW_REGS_SPAN 0x00200000 // I/O mapped Range
#define HW_REGS_MASK HW_REGS_SPAN-1 // mask bit

#define AUDIO_PIO_BASE 0x3040 // AUDIO I/O 의 상대 base 주소 : 0xFF203040

#define PI2 6.28318531 // sin 함수에서 음계 주파수를 만들기 위해 사용되는 PI 값
#define SAMPLE_RATE 48000 // DE1 SoC의 Sample Rate

void write_to_audio(double freq, volatile unsigned int* h2p_lw_audio_addr); // Audio 출력 함수
void musicplay(volatile unsigned int* h2p_lw_audio_addr); // 노래 출력 함수

unsigned char cont = 0;

// 음계에 대한 주파수를 저장해 둔 배열 변수
double tone[9] = {
	261.626, 293.655, 329.628, 349.228, 391.992, 440.000, 493.883, 523.251, 587.329,
};

// 노래 출력을 위한 좀 더 많은 음계 주파수 배열
double tune[20] = {
	261.626, 293.655, 329.628, 349.228, 391.992, 440.000, 493.883, 523.251, 587.329, 659.255, 698.457, 783.991, 880.0, 987.766, 1046.5,
};

int hex_display; // HEX Ãâ·Â : C D E F G A B 
int hex; // hex

int main() {
	int sw, data, rdata;

	//switch file open
	sw = open("/dev/sw", O_RDWR); // use read switch button
	if (sw < 0) { //if open failed
		fprintf(stderr, "Cannot open SW device.\n");
		return -1;
	}

	//hex file open
	hex = open("/dev/hex", O_RDWR); // use write
	if (hex < 0) { //if open failed
		fprintf(stderr, "cannot open HEX device.\n");
		return -1;
	}

	//audio part initialize
	volatile unsigned int* h2p_lw_audio_addr = NULL;

	void* virtual_base; // mapped µÈ Base ÁÖ¼Ò = 0xFF203040
	int fd; // Memory open variable

	//open /dev/mem
	if ((fd = open("/dev/mem", (O_RDWR | O_SYNC))) == -1) {
		printf(" ERROR : could not open /dev/mem.. \n");
		return 1;
	}

	//get virtual addr that maps to physical
	// mmap 함수를 사용하여 memory I/O mapping base : 0xFF200000 virtual mapping
	virtual_base = mmap(
		NULL,
		HW_REGS_SPAN,
		(PROT_READ | PROT_WRITE),
		MAP_SHARED,
		fd,
		HW_REGS_BASE
	);

	if (virtual_base == MAP_FAILED) {
		printf(" ERROR : mmap() failed... \n");
		close(fd);
		return 1;
	}

	//get virtual addr of AUDIO : 0xFF203040
	h2p_lw_audio_addr = (unsigned int*)(virtual_base + (AUDIO_PIO_BASE));

	//first, clear the WRITE FIFO by using the CW bit in Control Reg
	*(h2p_lw_audio_addr) = 0xC;     //CW=1, CR=1, WE=0, RE=0 // read, write FIFO 초기화­
	//and need to reset to 0 to use because CR, CW = 1 continuous clear FIFO
	*(h2p_lw_audio_addr) = 0x0; // read, write FIFO초기화 후 다시 '0' bit로 바꿔준다.

	double freq; // must have global variable

	//execute while loop
	while (1) {
		read(sw, &rdata, 4); // switch 값 읽기
		rdata = rdata & 0x3FF; // 10bit Mask

		// switch값에 따른 음계 출력
		switch (rdata) { 
			case 0x00:
				hex_display = 99;
				cont = 0;
				// Idle
				break;
			case 0x01:
				freq = tone[0];
				hex_display = 0;
				cont = 1;
				break;
			case 0x02:
				freq = tone[1];
				hex_display = 1;
				cont = 1;
				break;
			case 0x04:
				freq = tone[2];
				hex_display = 2;
				cont = 1;
				break;
			case 0x08:
				freq = tone[3];
				hex_display = 3;
				cont = 1;
				break;
			case 0x10:
				freq = tone[4];
				hex_display = 4;
				cont = 1;
				break;
			case 0x20:
				freq = tone[5];
				hex_display = 5;
				cont = 1;
				break;
			case 0x40:
				freq = tone[6];
				hex_display = 6;
				cont = 1;
				break;
			case 0x80:
				freq = tone[7];
				hex_display = 7;
				cont = 1;
				break;
			case 0x100:
				freq = tone[8];
				hex_display = 8;
				cont = 1;
				break;

			case 0x200:         //MSB Switch를 올리면 노래가 출력됩니다.
				musicplay(h2p_lw_audio_addr);
				hex_display = 11;
				sleep(1);
				break;

			default:
				hex_display = 11;
		}
		if (hex_display != 11 && hex_display != 99) {
			write_to_audio(freq, h2p_lw_audio_addr); // Audio 출력하는 함수
			usleep(50);
		}
	}

	//close
	close(sw);
	close(hex);
	return 0;
}

// Audio 출력 함수
void write_to_audio(double freq, volatile unsigned int* h2p_lw_audio_addr) {
	usleep(50); // micro seconds
	//write data in LeftData & RightData
	int nth_sample;
	printf("%f \n", freq);

	write(hex, &hex_display, 4); // device Driver fp name, data

	//Max volume when multiplied by sin() which ranges from -1 to 1
	int vol = 0x07FFFFFF; // volume : height of sin Function

	for (nth_sample = 0; nth_sample < SAMPLE_RATE*2; nth_sample++) {
		*(h2p_lw_audio_addr + 2) = vol * sin(nth_sample * freq * PI2 / (SAMPLE_RATE*2));
		*(h2p_lw_audio_addr + 3) = vol * sin(nth_sample * freq * PI2 / (SAMPLE_RATE*2));
	}
}

void write_to_audio_for_song(double freq, volatile unsigned int* h2p_lw_audio_addr, int count) {
	usleep(40); // micro seconds
	//write data in LeftData & RightData
	int nth_sample;
	printf("%f \n", freq);

	// sin 함수의 주파수 그래프는 Depth가 1 ~ -1 까지 이며 vol 변수 값을 곱함으로 써 진폭(Depth)를 늘려서 소리 크기를 조절 할 수 있다.
	int vol = 0x0FFFFFFF; // volume : height of sin Function

	// sin 함수를 사용해서 음계 주파수를 만들어내고 공식은 sin(freq * PI2 / SAMPLE_RATE)
	// sin 함수의 주기는 2PI 이며 'C'의 경우 262의 Hz를 가지는데 1초에 262개의 sin 파형이 그려져야 합니다.
	// DE1의 Sample rate는 48000Hz이므로 따라서, 'C'출력을 위해서는 frequency*PI2/Sample_Rate
	// 1초에 48000번 Sampling을 수행하며 이 때, sin 함수 262개의 주기가 들어가야 한다.
	// 따라서 262*PI*2/48000공식을 사용하여 48000번 Sampling 을 하면 262번의 sin 주기가 진행된다.
	for (nth_sample = 0; nth_sample < SAMPLE_RATE * count; nth_sample++) {
		*(h2p_lw_audio_addr + 2) = vol * sin(nth_sample * freq * PI2 / (SAMPLE_RATE * count));
		*(h2p_lw_audio_addr + 3) = vol * sin(nth_sample * freq * PI2 / (SAMPLE_RATE * count));
	}
}

//Alan Walker : Faded 곡 전반부 출력 
void musicplay(volatile unsigned int* h2p_lw_audio_addr) {
	write_to_audio_for_song(tune[3], h2p_lw_audio_addr, 6); //파
	usleep(1000 * 200);
	write_to_audio_for_song(tune[3], h2p_lw_audio_addr, 6);
	usleep(1000 * 200);
	write_to_audio_for_song(tune[3], h2p_lw_audio_addr, 6);
	usleep(1000 * 200);
	write_to_audio_for_song(tune[5], h2p_lw_audio_addr, 6);
	usleep(1000 * 200);

	write_to_audio_for_song(tune[8], h2p_lw_audio_addr, 6);
	usleep(1000 * 200);
	write_to_audio_for_song(tune[8], h2p_lw_audio_addr, 6);
	usleep(1000 * 200);
	write_to_audio_for_song(tune[8], h2p_lw_audio_addr, 6);
	usleep(1000 * 200);
	write_to_audio_for_song(tune[7], h2p_lw_audio_addr, 6);
	usleep(1000 * 200);


	write_to_audio_for_song(tune[5], h2p_lw_audio_addr, 6);
	usleep(1000 * 200);
	write_to_audio_for_song(tune[5], h2p_lw_audio_addr, 6);
	usleep(1000 * 200);
	write_to_audio_for_song(tune[5], h2p_lw_audio_addr, 6);
	usleep(1000 * 200);
	write_to_audio_for_song(tune[4], h2p_lw_audio_addr, 6);
	usleep(1000 * 200);


	write_to_audio_for_song(tune[2], h2p_lw_audio_addr, 6);
	usleep(1000 * 200);
	write_to_audio_for_song(tune[2], h2p_lw_audio_addr, 6);
	usleep(1000 * 200);
	write_to_audio_for_song(tune[2], h2p_lw_audio_addr, 6);
	usleep(1000 * 200);
	write_to_audio_for_song(tune[1], h2p_lw_audio_addr, 6);
	usleep(1000 * 200);

	write_to_audio_for_song(tune[10], h2p_lw_audio_addr, 3);
	usleep(1000 * 100);
	write_to_audio_for_song(tune[10], h2p_lw_audio_addr, 3);
	usleep(1000 * 100);
	write_to_audio_for_song(tune[8], h2p_lw_audio_addr, 3);
	usleep(1000 * 100);

	write_to_audio_for_song(tune[10], h2p_lw_audio_addr, 3);
	usleep(1000 * 100);
	write_to_audio_for_song(tune[8], h2p_lw_audio_addr, 3);
	usleep(1000 * 100);
	write_to_audio_for_song(tune[10], h2p_lw_audio_addr, 3);
	usleep(1000 * 100);
	write_to_audio_for_song(tune[11], h2p_lw_audio_addr, 3);
	usleep(1000 * 100);

	write_to_audio_for_song(tune[13], h2p_lw_audio_addr, 6);
	usleep(1000 * 100);
	write_to_audio_for_song(tune[10], h2p_lw_audio_addr, 6);
	usleep(1000 * 100);
	write_to_audio_for_song(tune[7], h2p_lw_audio_addr, 3);
	usleep(1000 * 100);
	write_to_audio_for_song(tune[13], h2p_lw_audio_addr, 9);
	usleep(1000 * 1000);

	write_to_audio_for_song(tune[10], h2p_lw_audio_addr, 3);
	usleep(1000 * 100);
	write_to_audio_for_song(tune[10], h2p_lw_audio_addr, 3);
	usleep(1000 * 100);
	write_to_audio_for_song(tune[10], h2p_lw_audio_addr, 3);
	usleep(1000 * 100);
	write_to_audio_for_song(tune[9], h2p_lw_audio_addr, 9);
	usleep(1000 * 100);

	write_to_audio_for_song(tune[9], h2p_lw_audio_addr, 3);
	usleep(1000 * 100);
	write_to_audio_for_song(tune[9], h2p_lw_audio_addr, 3);
	usleep(1000 * 100);
	write_to_audio_for_song(tune[8], h2p_lw_audio_addr, 3);
	usleep(1000 * 100);
	write_to_audio_for_song(tune[10], h2p_lw_audio_addr, 6);
	usleep(1000 * 100);

	write_to_audio_for_song(tune[10], h2p_lw_audio_addr, 3);
	usleep(1000 * 100);
	write_to_audio_for_song(tune[10], h2p_lw_audio_addr, 3);
	usleep(1000 * 100);
	write_to_audio_for_song(tune[8], h2p_lw_audio_addr, 3);
	usleep(1000 * 100);
	write_to_audio_for_song(tune[10], h2p_lw_audio_addr, 3);
	usleep(1000 * 100);
	write_to_audio_for_song(tune[8], h2p_lw_audio_addr, 3);
	usleep(1000 * 100);
	write_to_audio_for_song(tune[9], h2p_lw_audio_addr, 3);
	usleep(1000 * 100);
	write_to_audio_for_song(tune[10], h2p_lw_audio_addr, 3);
	usleep(1000 * 100);

	write_to_audio_for_song(tune[12], h2p_lw_audio_addr, 6);
	usleep(1000 * 100);
	write_to_audio_for_song(tune[10], h2p_lw_audio_addr, 3);
	usleep(1000 * 100);
	write_to_audio_for_song(tune[10], h2p_lw_audio_addr, 3);
	usleep(1000 * 100);
	write_to_audio_for_song(tune[14], h2p_lw_audio_addr, 3);
	usleep(1000 * 100);
	write_to_audio_for_song(tune[12], h2p_lw_audio_addr, 9);
	usleep(1000 * 1000);

	write_to_audio_for_song(tune[10], h2p_lw_audio_addr, 3);
	usleep(1000 * 100);
	write_to_audio_for_song(tune[10], h2p_lw_audio_addr, 3);
	usleep(1000 * 100);
	write_to_audio_for_song(tune[11], h2p_lw_audio_addr, 3);
	usleep(1000 * 100);
	write_to_audio_for_song(tune[11], h2p_lw_audio_addr, 12);
	usleep(1000 * 200);

	write_to_audio_for_song(tune[12], h2p_lw_audio_addr, 3);
	usleep(1000 * 100);
	write_to_audio_for_song(tune[12], h2p_lw_audio_addr, 3);
	usleep(1000 * 100);
	write_to_audio_for_song(tune[12], h2p_lw_audio_addr, 3);
	usleep(1000 * 100);
	write_to_audio_for_song(tune[12], h2p_lw_audio_addr, 6);
	usleep(1000 * 100);

	write_to_audio_for_song(tune[10], h2p_lw_audio_addr, 6);
	usleep(1000 * 100);
	write_to_audio_for_song(tune[10], h2p_lw_audio_addr, 6);
	usleep(1000 * 100);
	write_to_audio_for_song(tune[12], h2p_lw_audio_addr, 6);
	usleep(1000 * 100);

	write_to_audio_for_song(tune[15], h2p_lw_audio_addr, 6);
	usleep(1000 * 100);
	write_to_audio_for_song(tune[15], h2p_lw_audio_addr, 6);
	usleep(1000 * 100);
	write_to_audio_for_song(tune[15], h2p_lw_audio_addr, 3);
	usleep(1000 * 100);
	write_to_audio_for_song(tune[12], h2p_lw_audio_addr, 3);
	usleep(1000 * 100);
	write_to_audio_for_song(tune[12], h2p_lw_audio_addr, 3);
	usleep(1000 * 100);
	write_to_audio_for_song(tune[12], h2p_lw_audio_addr, 3);
	usleep(1000 * 100);

}
//2015253039 권진우
