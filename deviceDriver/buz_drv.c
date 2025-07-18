#include <linux/fs.h> 			/* open( ), read( ), write( ), close( ) 커널 함수 */
#include <linux/cdev.h> 		/* 문자 디바이스 */
#include <linux/module.h>
#include <linux/io.h> 			/* ioremap( ), iounmap( ) 커널 함수 */
#include <linux/uaccess.h> 		/* copy_to_user( ), copy_from_user( ) 커널 함수 */
#include <linux/gpio.h>			/* GPIO 함수 */
#include <linux/delay.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SUUB");
MODULE_DESCRIPTION("Raspberry Pi GPIO BUZ Device Module");

#if 0
//#define BCM_IO_BASE 0x20000000 	/* Raspberry Pi B/B+의 I/O Peripherals 주소 */
#define BCM_IO_BASE 0x3F000000 		/* Raspberry Pi 2/3의 I/O Peripherals 주소 */
#else
#define BCM_IO_BASE 0xFE000000 		/* Raspberry Pi 4의 I/O Peripherals 주소 */
#endif

#define GPIO_BASE (BCM_IO_BASE + 0x200000) 	/* GPIO 컨트롤러의 주소 */
#define GPIO_SIZE (256) 		/* 0x7E2000B0 - 0x7E2000000 + 4 = 176 + 4 = 180 */

/* GPIO 설정 매크로 */
#define GPIO_IN(g) (*(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))) 	/* 입력 설정 */
#define GPIO_OUT(g) (*(gpio+((g)/10)) |= (1<<(((g)%10)*3))) 	/* 출력 설정 */
#define GPIO_SET(g) (*(gpio+7) = 1<<g) 				/* 비트 설정 */
#define GPIO_CLR(g) (*(gpio+10) = 1<<g) 			/* 설정된 비트 해제 */
#define GPIO_GET(g) (*(gpio+13)&(1<<g)) 			/* 현재 GPIO의 비트에 대한 정보 획득 */

/* 디바이스 파일의 주 번호와 부 번호 */
#define GPIO_MAJOR 203
#define GPIO_MINOR 0
#define GPIO_DEVICE "gpiobuz" 		/* 디바이스 파일의 이름 */

#define GPIO_BUZ 537 			/* 부저 사용을 위한 GPIO의 번호 */

//static char msg[BLOCK_SIZE] = {0}; 	/* write( ) 함수에서 읽은 데이터 저장 */

/* 입출력 함수를 위한 선언 */
static int gpio_open(struct inode *, struct file *);
static ssize_t gpio_write(struct file *, const char *, size_t, loff_t *);
static int gpio_close(struct inode *, struct file *);

/* 유닉스 입출력 함수들의 처리를 위한 구조체 */
static struct file_operations gpio_fops = {
    .owner = THIS_MODULE,
    .write = gpio_write,
    .open = gpio_open,
    .release = gpio_close,
};

struct cdev gpio_cdev;

int init_module(void)
{
    dev_t devno;
    unsigned int count;
    int err;

    printk(KERN_INFO "Hello buz!\n");
    // try_module_get(THIS_MODULE);

    /* 문자 디바이스를 등록한다. */
    devno = MKDEV(GPIO_MAJOR, GPIO_MINOR);
    register_chrdev_region(devno, 1, GPIO_DEVICE);

    /* 문자 디바이스를 위한 구조체를 초기화한다. */
    cdev_init(&gpio_cdev, &gpio_fops);

    gpio_cdev.owner = THIS_MODULE;
    count = 1;
    err = cdev_add(&gpio_cdev, devno, count); 		/* 문자 디바이스를 추가한다. */
    if (err < 0) {
        printk("Error : Device Add\n");
        return -1;
    }

    printk("'mknod /dev/%s c %d 0'\n", GPIO_DEVICE, GPIO_MAJOR);
    printk("'chmod 666 /dev/%s'\n", GPIO_DEVICE);

    /* GPIO 사용을 요청한다. */
    gpio_request(GPIO_BUZ, "buz");
    gpio_direction_output(GPIO_BUZ, 0);

    return 0;
}

void cleanup_module(void)
{
    dev_t devno = MKDEV(GPIO_MAJOR, GPIO_MINOR);
    unregister_chrdev_region(devno, 1); 		/* 문자 디바이스의 등록을 해제한다. */

    cdev_del(&gpio_cdev); 				/* 문자 디바이스의 구조체를 해제한다. */

    /* 더 이상 사용이 필요 없는 경우 관련 자원을 해제한다. */
    gpio_free(GPIO_BUZ);
    gpio_direction_output(GPIO_BUZ, 0);

    // module_put(THIS_MODULE);

    printk(KERN_INFO "Good-bye module!\n");
}

static int gpio_open(struct inode *inod, struct file *fil)
{
    printk("GPIO Device opened(%d/%d)\n", imajor(inod), iminor(inod));

    return 0;
}

static int gpio_close(struct inode *inod, struct file *fil)
{
    printk("GPIO Device closed(%d)\n", MAJOR(fil->f_path.dentry->d_inode->i_rdev));

    return 0;
}

static ssize_t gpio_write(struct file *inode, const char *buff, size_t len, loff_t *off)
{
    char kbuf[32] = {0};
    int freq = 0, dur = 0;  // Hz, ms
    int half_period, cycles;

    if (len > sizeof(kbuf) - 1)
        len = sizeof(kbuf) - 1;

    // 사용자 공간 → 커널 공간 복사
    // copy_from_user → sscanf("440 300", &freq, &dur)
    if (copy_from_user(kbuf, buff, len))
        return -EFAULT;

    kbuf[len] = '\0';

    // 문자열 파싱
    if (sscanf(kbuf, "%d %d", &freq, &dur) != 2 || freq <= 0 || dur <= 0) {
        printk("Invalid input: expected \"<freq> <duration>\"\n");
        return -EINVAL;
    }

    // PWM 방식 사각파 출력
    half_period = 500000 / freq;
    cycles = (dur * 1000) / (2 * half_period);

    for (int i = 0; i < cycles; i++) {
        gpio_set_value(GPIO_BUZ, 1);
        udelay(half_period);
        gpio_set_value(GPIO_BUZ, 0);
        udelay(half_period);
    }

    printk("Buzzer: %d Hz for %d ms\n", freq, dur);
    return len;
}
