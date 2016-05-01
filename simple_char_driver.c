#include<linux/init.h>
#include<linux/module.h>

#include<linux/fs.h>
#include<asm/uaccess.h>

#define BUFFER_SIZE 1024

/**
 * @brief The Buffer used for the copy 
 * @note Allocated on the Static Data Segment 
 * @note Linker visibility is limited to the current file 
 */
static char device_buffer[BUFFER_SIZE];

int openCount = 0;
int closeCount = 0;
int placeholder = 0;

/**
 * @brief It moves the data from the internal `device_buffer` to the user space `buffer` in multiple iterations 
 * @param pfile Driver File. TBD: If not used why passed ? 
 * @note Interface IN: copy_to_user() is the function actually performing the copy into the User Space of certain amount of byte (i.e. probably not the full buffer)
 * @note Interface OUT: the amount of bytes copyed in this iteration 
 * @note: It should be used by another module in a cycle to make sure the full copy has been performed 
 */
ssize_t simple_char_driver_read (struct file *pfile, char __user *buffer, size_t length, loff_t *offset)
{
	/* pfile = driver file
	 * buffer = buffer in user space
	 * length = length of what is to be read (using cat will give something like 6000+ bytes)
	 * offset = variable to store offset for ONE read cycle.
	 */
	 
	/**
	  * @brief The Amount of Data Read 
	  */
	int bytesRead;
	
	/**
	  * @brief The Amount to be Read 
	  * @note The `offset` represents the pointer to the first byte to be read 
	  */ 
	int bytesToRead = BUFFER_SIZE - *offset;

	// If we are at the end of the file, STOP READING!
	if (bytesToRead == 0){
		printk(KERN_ALERT "Reached the end of the file");
		return bytesToRead;
	}
	
	// Get bytes read by subtracting return of copy_to_user (returns unread bytes)
	//** Every time it tries to copy the full amount of remaining bytes 
	//** TBD: Maybe it could be provided a faster multithreading implementation 
	bytesRead = bytesToRead - copy_to_user(buffer, device_buffer + *offset, bytesToRead);
	printk(KERN_ALERT "READING with Simple Character Driver. Reading %d bytes\n", bytesRead);

	// Set offset so that we can eventually reach the end of the file
	*offset += bytesRead;

	return bytesRead;
}



ssize_t simple_char_driver_write (struct file *pfile, const char __user *buffer, size_t length, loff_t *offset)
{
	/* pfile = driver file
	 * buffer = buffer in user space
	 * length = length of what is to be written
	 * offset = variable to store offset for ONE write
	 * placeholder = global var for the current write spot in the buffer
	 */

	
	int bytesToWrite;
	int bytesWritten;
	int bytesAvailable = BUFFER_SIZE - *offset - placeholder;

	// Make sure there is sufficient space
	if(bytesAvailable > length){
		bytesToWrite = length; 
	}
	else{
		bytesToWrite = bytesAvailable;
	}

	//Get bites written by subtracting unwritten bites from retun of copy_from_user
	bytesWritten = bytesToWrite - copy_from_user(device_buffer + *offset + placeholder, buffer, bytesToWrite);
	
	// If no space left:
	if(bytesWritten == 0){
		printk(KERN_ALERT "The device is out of space.\n");
	}
	else{
		//Increment offset and placeholder
		*offset += bytesWritten;
		placeholder += bytesWritten;

		printk(KERN_ALERT "WRITING with Simple Character Driver. Writing %d bytes\n", bytesWritten);
	}
	return bytesWritten;
}


int simple_char_driver_open (struct inode *pinode, struct file *pfile)
{
	/* print to the log file that the device is opened and also print the number of times this device has been opened until now*/
	openCount++;
	printk(KERN_ALERT "OPENING Simple Character Driver. It has been opened %d times\n", openCount);
	return 0;
}


int simple_char_driver_close (struct inode *pinode, struct file *pfile)
{
	/* print to the log file that the device is closed and also print the number of times this device has been closed until now*/
	closeCount++;
	printk(KERN_ALERT "CLOSING Simple Character Driver. It has been closed %d times\n", closeCount);
	return 0;
}

struct file_operations simple_char_driver_file_operations = {
	.owner   = THIS_MODULE,
	.read    = simple_char_driver_read,
	.write   = simple_char_driver_write,
	.open    = simple_char_driver_open,
	.release = simple_char_driver_close
};

static int simple_char_driver_init(void)
{
	/* print to the log file that the init function is called.*/
	printk(KERN_ALERT "INITIALIZING Simple Character Driver\n");
	/* register the device */
	register_chrdev( 301, "simple_driver", &simple_char_driver_file_operations);
	return 0;
}

// Return type must be void to avoid warning in compilation
static void simple_char_driver_exit(void)
{
	/* print to the log file that the exit function is called.*/
	printk(KERN_ALERT "EXITING Simple Character Driver\n");
	/* unregister the device */
	unregister_chrdev( 301, "simple_driver");
}

/* add module_init and module_exit to point to the corresponding init and exit function*/

module_init(simple_char_driver_init);
module_exit(simple_char_driver_exit);
