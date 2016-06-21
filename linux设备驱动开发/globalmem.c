#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define GLOBALMEM_SIZE                    (0x1000)
#define MEM_CLEAR                         (0x1)
#define GLOBALMEM_MAJOR                   (230)
#define DEVICE_NUM                        (10)

static int globalmem_major = GLOBALMEM_MAJOR;
module_param(globalmem_major, int, S_IRUGO);

struct globalmem_dev {
    struct cdev cdev;
    unsigned char mem[GLOBALMEM_SIZE];
    struct mutex mutex;
};

static ssize_t globalmem_read(struct file* flip, char __user * buf, size_t size, loff_t* ppos)
{
    unsigned long p = *ppos;
    unsigned int count = size;

    int ret = 0;
    struct globalmem_dev* dev = flip->private_data;

    if (p >= GLOBALMEM_SIZE)
        return 0;

    if (count > GLOBALMEM_SIZE - p)
        count = GLOBALMEM_SIZE - p;

    mutex_lock(&dev->mutex);
    
    if (copy_to_user(buf, dev->mem + p, count))
    {
        return -EFAULT;
    }
    else
    {
        *ppos += count;
        ret = count;

        printk(KERN_INFO "read %u bytes(s) from %lu\n", count, p);
    }

    mutex_unlock(&dev->mutex);

    return ret;
}

static ssize_t globalmem_write(struct file* flip, char __user * buf, size_t size, loff_t* ppos)
{
    unsigned long p = *ppos;
    unsigned int count = size;

    int ret = 0;
    struct globalmem_dev* dev = flip->private_data;

    if (count > GLOBALMEM_SIZE - p)
        count = GLOBALMEM_SIZE - p;

    mutex_lock(&dev->mutex);
    
    if (copy_from_user(dev->mem + p, buf, count))
    {
        return -EFAULT;
    }
    else
    {
        *ppos += count;
        ret = count;

        printk(KERN_INFO "write %u bytes(s) from %lu\n", count, p);
    }
    
    mutex_unlock(&dev->mutex);

    return ret;    
}

static loff_t globalmem_llseek(struct file* flip, loff_t offset, int orig)
{
    loff_t ret = 0;

    switch (orig)
    {
    case 0:
        if (offset < 0)
        {
            ret = -EINVAL;
            break;
        }

        if ((unsigned int)offset > GLOBALMEM_SIZE)
        {
            ret = -EINVAL;
            break;
        }
        
        flip->f_ops = (unsigned int)offset;
        ret = flip->f_ops;
        break;

    case 1:
        if ((flip->f_ops + offset) > GLOBALMEM_SIZE)
        {
            ret = -EINVAL;
            break;
        }

        if ((flip->f_ops + offset) < 0)
        {
            ret = -EINVAL;
            break;
        }
        flip->f_ops += offset;
        ret = flip->f_ops;
        break;

    default:
        ret = -EINVAL;
        break;
    }

    return ret;
}

static long globalmem_ioctl(struct file* flip, unsigned int cmd, unsigned long arg)
{
    struct globalmem_dev* dev = flip->private_data;

    switch (cmd)
    {
    case MEM_CLEAR:
        mutex_lock(&dev->mutex);
        memset(dev->mem, 0, GLOBALMEM_SIZE);        
        mutex_unlock(&dev->mutex);
        
        printk(KERN_INFO "globalmem is set to zero\n");
        break;

    default:
        return -EINVAL;
    }

    return 0;
}

static int globalmem_open(struct inode* inode, struct file* flip)
{
    struct globalmem_dev* dev = container_of(inode->icdev, struct globalmem_dev, cdev);
    
    flip->private = dev;

    return 0;
}

static int globalmem_release(struct inode* inode, struct file* flip)
{
    return 0;
}

static const struct file_operations globalmem_fops = {
    .owner          = THIS_MODULE;
    .llseek         = globalmem_llseek;
    .read           = globalmem_read;
    .write          = globalmem_write;
    .unlocked_ioctl = globalmem_ioctl;
    .open           = globalmem_open;
    .release        = globalmem_release;
};


struct globalmem_dev* globalmem_devp;

static void globalmem_setup_cdev(struct globalmem_dev* dev, int index)
{
    int err;
    int devno = MKDEV(globalmem_major, index);

    cdev_init(&dev->cdev, globalmem_fops);

    dev->cdev.owner = THIS_MODULE;
    err = cdev_add(&dev->cdev, devno, 1);
    if (err)
        printk(KERN_NOTICE "Error %d adding globalmem%d", err, index);
}

static int __init globalmem_init(void)
{
    int i;
    int ret;
    dev_t devno = MKDEV(globalmem_major, 0);

    if (globalmem_major)
    {
        ret = register_chrdev_region(devno, 1, "globalmem");
    }
    else
    {
        ret = alloc_chrdev_region(&devno, 0, 1, "globalmem");
        globalmem_major = MAJOR(devno);
    }

    if (ret < 0)
        return ret;

    globalmem_devp = kzalloc(sizeof(struct globalmem_dev), GFP_KERNEL);
    if (!globalmem_devp)
    {
        ret = -ENOMEM;
        goto fail_malloc;
    }

    for (i = 0; i < DEVICE_NUM; i++)
    {
        mutex_init(&globalmem_devp->mutex);
        globalmem_setup_cdev(globalmem_devp, i);
    }

    return 0;

fail_malloc:
    unregister_chrdev_region(devno, 1);
    
    return ret;
}
module_init(globalmem_init);

static int __exit globalmem_exit(void)
{
    int i;
    dev_t devno = MKDEV(globalmem_major, 0);

    for (i = 0; i < DEVICE_NUM; i++)
        cdev_del(&globalmem_dev->cdev);

    kfree(globalmem_devp);

    unregister_chrdev_region(devno, 1);
}
module_exit(globalmem_exit);


MODULE_AUTHOR("ZLG");
MODULE_LICENSE("GPL v2");




