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
MODULE_DESCRIPTION("SW");

#define base_lwFPGA 0xFF200000
#define len_lwFPGA 0x00200000

#define addr_SW 0x40

#define SW_DEVMAJOR 238
#define SW_DEVNAME "sw"

static void* mem_base;
static void* sw_addr;

//open operation
static int sw_open(struct inode* minode, struct file* mfile) {
	//do nothing.
	return 0;
}

//release operation
static int sw_release(struct inode* minode, struct file* mfile) {
	//do nothing.
	return 0;
}

//write operation
static ssize_t sw_write(struct file* file, const int __user* buf, size_t count, loff_t* f_ops) {
	//do nothing.
	return 0;
}

//read operation
//switch 값을 읽어서 현재 switch 값을 받고 0x3FF 와 Mask Op을 수행하여 10bit 값만 추출한다.
static ssize_t sw_read(struct file* file, const int __user* buf, size_t count, loff_t* f_ops) {
	unsigned int sw_data = 0;
	int ret;

	sw_data = ioread32(sw_addr);
	sw_data = sw_data & 0x3FF;

	put_user(sw_data, (unsigned int*)buf); //only works on 2byte(16bit)
	//ret = copy_to_user(buf, &sw_data, count);

	return 4;
}

// file Operations struct
static struct file_operations sw_fops = {
   .read = sw_read,
   .write = sw_write,
   .open = sw_open,
   .release = sw_release,
};

//driver init
static int __init sw_init(void) {
	int res;

	res = register_chrdev(SW_DEVMAJOR, SW_DEVNAME, &sw_fops);

	if (res < 0) {
		printk(KERN_ERR " sw : failed to register device. \n");
		return res;
	}

	// mapping address
	mem_base = ioremap_nocache(base_lwFPGA, len_lwFPGA);

	if (!mem_base) {
		printk("Error mapping memory. \n");
		release_mem_region(base_lwFPGA, len_lwFPGA);
		return -EBUSY;
	}

	sw_addr = mem_base + addr_SW;

	printk("Device : %s Major : %d \n", SW_DEVNAME, SW_DEVMAJOR);
	return 0;
}

static void __exit sw_exit(void) {
	unregister_chrdev(SW_DEVMAJOR, SW_DEVNAME);
	printk(" %s unregistered. \n", SW_DEVNAME);
	iounmap(mem_base);      //virtual mem release
}

module_init(sw_init);
module_exit(sw_exit);
