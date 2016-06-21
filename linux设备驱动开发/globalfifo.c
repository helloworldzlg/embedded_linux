#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define GLOBALFIFO_SIZE                    (0x1000)
#define MEM_CLEAR                          (0x1)
#define GLOBALFIFO_MAJOR                   (230)
#define DEVICE_NUM                         (10)

static int globalfifo_major = GLOBALFIFO_MAJOR;
module_param(globalfifo_major, int, S_IRUGO);

struct globalfifo_dev {
    struct cdev cdev;
    unsigned int current_len;
    unsigned char mem[GLOBALFIFO_SIZE];
    struct mutex mutex;
    wait_queue_head_t r_wait;
    wait_queue_head_t w_wait;    
};

static ssize_t globalfifo_read(struct file* flip, char __user * buf, size_t size, loff_t* ppos)
{
    unsigned long p = *ppos;
    unsigned int count = size;

    int ret = 0;
    struct globalfifo_dev* dev = flip->private_data;

    if (p >= GLOBALFIFO_SIZE)
        return 0;    
        
    DECLARE_WAITQUEUE(wait, current);

    mutex_lock(&dev->mutex);
    
    add_wait_queue(&dev->r_wait, wait);

    while (dev->current_len == 0)
    {
        if (flip->f_flags & O_NONBLOCK)
        {
            ret = -EAGAIN;
            goto out;
        }
    }

    __set_current_state(TASK_INTERRUPTIBLE);
    
    mutex_unlock(&dev->mutex);

    schedule();

    if (signal_pending(current))
    {
        ret = -ERESTARTSYS;
        goto out2;
    }

    mutex_lock(&dev->mutex);

    if (count > dev->current_len)
        count = dev->current_len;
    
    if (copy_to_user(buf, dev->mem + p, count))
    {
        return -EFAULT;
        goto out;
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

static ssize_t globalfifo_write(struct file* flip, char __user * buf, size_t size, loff_t* ppos)
{
    unsigned long p = *ppos;
    unsigned int count = size;

    int ret = 0;
    struct globalfifo_dev* dev = flip->private_data;

    if (count > GLOBALFIFO_SIZE - p)
        count = GLOBALFIFO_SIZE - p;

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

static loff_t globalfifo_llseek(struct file* flip, loff_t offset, int orig)
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

        if ((unsigned int)offset > GLOBALFIFO_SIZE)
        {
            ret = -EINVAL;
            break;
        }
        
        flip->f_ops = (unsigned int)offset;
        ret = flip->f_ops;
        break;

    case 1:
        if ((flip->f_ops + offset) > GLOBALFIFO_SIZE)
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

static long globalfifo_ioctl(struct file* flip, unsigned int cmd, unsigned long arg)
{
    struct globalfifo_dev* dev = flip->private_data;

    switch (cmd)
    {
    case MEM_CLEAR:
        mutex_lock(&dev->mutex);
        memset(dev->mem, 0, GLOBALFIFO_SIZE);        
        mutex_unlock(&dev->mutex);
        
        printk(KERN_INFO "globalmem is set to zero\n");
        break;

    default:
        return -EINVAL;
    }

    return 0;
}

static int globalfifo_open(struct inode* inode, struct file* flip)
{
    struct globalfifo_dev* dev = container_of(inode->icdev, struct globalfifo_dev, cdev);
    
    flip->private = dev;

    return 0;
}

static int globalfifo_release(struct inode* inode, struct file* flip)
{
    return 0;
}

static const struct file_operations globalfifo_fops = {
    .owner          = THIS_MODULE;
    .llseek         = globalfifo_llseek;
    .read           = globalfifo_read;
    .write          = globalfifo_write;
    .unlocked_ioctl = globalfifo_ioctl;
    .open           = globalfifo_open;
    .release        = globalfifo_release;
};


struct globalfifo_dev* globalfifo_devp;

static void globalfifo_setup_cdev(struct globalfifo_dev* dev, int index)
{
    int err;
    int devno = MKDEV(globalfifo_major, index);

    cdev_init(&dev->cdev, globalfifo_fops);

    dev->cdev.owner = THIS_MODULE;
    err = cdev_add(&dev->cdev, devno, 1);
    if (err)
        printk(KERN_NOTICE "Error %d adding globalfifo%d", err, index);
}

static int __init globalfifo_init(void)
{
    int i;
    int ret;
    dev_t devno = MKDEV(globalfifo_major, 0);

    if (globalfifo_major)
    {
        ret = register_chrdev_region(devno, 1, "globalfifo");
    }
    else
    {
        ret = alloc_chrdev_region(&devno, 0, 1, "globalfifo");
        globalfifo_major = MAJOR(devno);
    }

    if (ret < 0)
        return ret;

    globalfifo_devp = kzalloc(sizeof(struct globalfifo_dev), GFP_KERNEL);
    if (!globalfifo_devp)
    {
        ret = -ENOMEM;
        goto fail_malloc;
    }

    for (i = 0; i < DEVICE_NUM; i++)
    {
        mutex_init(&globalfifo_devp->mutex);
        init_waitqueue_head(&globalfifo_devp->r_wait);
        init_waitqueue_head(&globalfifo_devp->w_wait);
        globalfifo_setup_cdev(globalfifo_devp, i);
    }

    return 0;

fail_malloc:
    unregister_chrdev_region(devno, 1);
    
    return ret;
}
module_init(globalfifo_init);

static int __exit globalfifo_exit(void)
{
    int i;
    dev_t devno = MKDEV(globalfifo_major, 0);

    for (i = 0; i < DEVICE_NUM; i++)
        cdev_del(&globalfifo_dev->cdev);

    kfree(globalfifo_devp);

    unregister_chrdev_region(devno, 1);
}
module_exit(globalfifo_exit);


MODULE_AUTHOR("ZLG");
MODULE_LICENSE("GPL v2");




