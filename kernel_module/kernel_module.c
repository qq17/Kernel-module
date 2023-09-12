#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/timer.h>
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/limits.h>
#include <linux/string.h>
#include <linux/err.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Maxim Ilchenko, ST-SPB-29883");
MODULE_DESCRIPTION("Kernel module with timer, appends files with lines by timeout");

static struct kobject *kobj;
static struct timer_list my_timer;
static unsigned int period = 5;
static char path[PATH_MAX] = "/tmp/hello"; 
static struct file* fd;
static size_t res;
static const char * msg = "Hello from kernel module\n";
static int size;
static int error = 0;

// дописывает в конец файла path строки "Hello from kernel module\n"
static int append_file(char * path)
{
	size = strlen(msg);
	fd = filp_open(path, O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if (IS_ERR(fd) || fd == NULL)
       	{
		// ошибки открытия файлов возникают, если путь указан неверно,
		// или директория, в которой лежит файл, не существует,
		// или если владельцем файла является не root
		printk(KERN_ALERT "Can't open file %s\nError code: %ld\n", path, PTR_ERR(fd));
		return PTR_ERR(fd);
	}
	res = kernel_write(fd, msg, size, &fd->f_pos);
	if (res < 0)
	{
		printk(KERN_ALERT "Error kernel_write to file %s\n", path);
		return res;
	}
	printk(KERN_INFO "%ld bytes written to %s\n", res, path);
	filp_close(fd, NULL);
	return 0;
}

// для задания параметров модуля ядра используется sysfs
// метод чтения атрибута period
static ssize_t period_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", period);
}

// метод записи атрибута period
static ssize_t period_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	sscanf(buf, "%u\n", &period);
	return count;
}

// метод чтения атрибута path
static ssize_t path_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", path);
}

// метод записи атрибута path
static ssize_t path_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	sscanf(buf, "%s\n", path);
	return count;
}

// атрибуты
static struct kobj_attribute k_attr_period = __ATTR(period, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH, period_show, period_store);
static struct kobj_attribute k_attr_path = __ATTR(path, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH, path_show, path_store);

// функция, вызываемая при срабатывании таймера
void my_timer_callback(struct timer_list *timer)
{
	//возобновление таймера
	mod_timer(&my_timer, jiffies + msecs_to_jiffies(period * 1000));
	printk(KERN_INFO "Current period is %d seconds\n", period);
	append_file(path);
}

static int init_kernel_module(void)
{
	printk(KERN_INFO "Initializing the kernel module\n");

	kobj = kobject_create_and_add("kernel_module", kernel_kobj);
	if (!kobj)
	{
		printk(KERN_ALERT "Error kobject_create_and_add()\n");
		return -ENOMEM;
	}

	error = sysfs_create_file(kobj, &k_attr_period.attr);
	if (error)
	{
		printk(KERN_ALERT "Error sysfs_create_file() for period\n");
		return error;
	}

	error = sysfs_create_file(kobj, &k_attr_path.attr);
	if (error)
	{
		printk(KERN_ALERT "Error sysfs_create_file() for path\n");
		return error;
	}

	timer_setup(&my_timer, my_timer_callback, 0);
	mod_timer(&my_timer, jiffies + msecs_to_jiffies(period * 1000));
	return 0;
}

static void exit_kernel_module(void)
{
	printk(KERN_INFO "Exiting the kernel module\n");
	del_timer_sync(&my_timer);
	kobject_put(kobj);
}

module_init(init_kernel_module);
module_exit(exit_kernel_module);
