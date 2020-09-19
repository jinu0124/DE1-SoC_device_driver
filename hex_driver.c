#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <asm/io.h>

#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/types.h>
#include <linux/ioport.h>

#include <linux/sched.h>

MODULE_LICENSE("GPL"); //LICENSE
MODULE_AUTHOR("KWON JIN WOO");
MODULE_DESCRIPTION("HEX");

#define base_lwFPGA 0xFF200000 // memory mapped I/O base address
#define len_lwFPGA 0x00200000 // memory mapped I/O range

#define addr_HEX3HEX0 0x20 // hex3~0 (8bit씩 4개의 HEX LED)
#define addr_HEX5HEX4 0x30 // hex5,4

#define HEX_DEVMAJOR 240 // Device Major number
#define HEX_DEVNAME "hex" // Device name

static void* mem_base; // Memory Mapped base address를 mapping
static void* hex3hex0_addr;
static void* hex5hex4_addr;
unsigned int in_hex3 = 0; // HEX3에 들어있는 값을 HEX4로 올려주기 위한 변수
unsigned int pre_data = 0; // HEX LED의 이전 DATA

unsigned int hex3hex0_data;
unsigned int hex5hex4_data;

int hex_conversions[10] = {
	0x39, 0x5E, 0x79, 0x71, 0x7D, 0x77, 0x7C, 0x39, 0x5E, 0x3F, //C D E F G A B C D
};

//open operation
static int hex_open(struct inode* minode, struct file* mfile) {
	//do nothing
	return 0;
}

//release operation
static int hex_release(struct inode* minode, struct file* mfile) {
	//do nothing
	return 0;
}

//write operation
static int hex_write(struct file* file, const int __user* buf, size_t count, loff_t* f_pos) {
	unsigned int hex_data = 0;  //32bit data

	int ret;

	unsigned int hex_data_set;   //32bit data
	unsigned int hex_data_set2;  //32bit data

	// get data from buffer
	get_user(hex_data, (unsigned int*)buf);  // Switch의 입력을 받아서 hex_data애 해당 bit 값 넣기
	in_hex3 = pre_data >> 24 & 0xFF; // 24bit right shift 후 Mask 연산을 사용하여 HEX3의 값 구하기
	pre_data = pre_data << 8 | hex_data; // pre_data에 새로 들어온 hex_data와 기존의 data를 한칸씩 좌측으로 밀어서 저장한다.


	hex_data_set2 = (in_hex3 & 0xFF);
	
	hex_data_set = pre_data;

	do {
		hex3hex0_data = hex_conversions[(hex_data_set & 0xFF)];
		hex3hex0_data |= hex_conversions[(hex_data_set >> 8 & 0xFF)] << 8;
		hex3hex0_data |= hex_conversions[(hex_data_set >> 16 & 0xFF)] << 16;
		hex3hex0_data |= hex_conversions[(hex_data_set >> 24 & 0xFF)] << 24;

		hex5hex4_data = hex_conversions[(hex_data_set2 & 0xFF)];
		hex5hex4_data |= hex_conversions[9] << 8;

	} while (0);

	//I/O mapped 된 hex LED주소에 hex data 값을 write 한다.
	iowrite32(hex3hex0_data, hex3hex0_addr);
	iowrite32(hex5hex4_data, hex5hex4_addr);

	return 4;
}

//read opeartion
static int hex_read(struct file* file, char __user* buf, size_t count, loff_t* f_pos) {
	//do nothing
	return 0;
}

//file operation initialize
static struct file_operations hex_fops = {
	.read = hex_read,
	.write = hex_write,
	.open = hex_open,
	.release = hex_release,
};

//driver init
static int __init hex_init(void) {
	int res;
	// device driver 등록
	res = register_chrdev(HEX_DEVMAJOR, HEX_DEVNAME, &hex_fops);
	if (res < 0) {
		printk(KERN_ERR " hex : failed to register device. \n");
		return res;
	}

	//mapping address
	mem_base = ioremap_nocache(base_lwFPGA, len_lwFPGA);

	if (!mem_base) {
		printk("Error mapping memory!\n");
		release_mem_region(base_lwFPGA, len_lwFPGA);
		return -EBUSY;
	}

	hex3hex0_addr = mem_base + addr_HEX3HEX0;
	hex5hex4_addr = mem_base + addr_HEX5HEX4;

	// 초기화
	iowrite32(0, hex3hex0_addr);
	iowrite32(0, hex5hex4_addr);

	printk(" Device : %s MAJOR : %d \n", HEX_DEVNAME, HEX_DEVMAJOR);
	return 0;
}

static void __exit hex_exit(void) {
	iowrite32(0, hex3hex0_addr);
	iowrite32(0, hex5hex4_addr); // HEX LED OFF

	unregister_chrdev(HEX_DEVMAJOR, HEX_DEVNAME);

	printk(" %s unreginsered. \n", HEX_DEVNAME);
	iounmap(mem_base);
}

module_init(hex_init);
module_exit(hex_exit);
