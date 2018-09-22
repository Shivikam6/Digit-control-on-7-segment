#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>       // Required for the GPIO functions
#include <linux/interrupt.h>  // Required for the IRQ code
#include <asm/uaccess.h>
#include <linux/mutex.h>
#include <linux/device.h>         // Header to support the kernel Driver Model
#include <linux/fs.h>             // Header for the Linux file system support
#define  CLASS_NAME  "char"
#define  DEVICE_NAME "leddriver" 

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Shivika Malik");
MODULE_DESCRIPTION("A 7 Segment Display test driver for the BBB");
MODULE_VERSION("1.0");

static unsigned int segA = 47; //Display pin 14
static unsigned int segB = 117; //Display pin 16
static unsigned int segC = 115; //Display pin 13
static unsigned int segD = 48; //Display pin 3
static unsigned int segE = 44; //Display pin 5
static unsigned int segF = 26; //Display pin 11
static unsigned int segG = 27; //Display pin 15
static unsigned int dig1 = 20; //Display pin 1
//static unsigned int dig2 = 67; //Display pin 2
//static unsigned int dig3 = 68; //Display pin 6
//static unsigned int dig4 = 44; //Display pin 8
static unsigned int gpioButton = 48;   ///< hard coding the button gpio for this example to P9_15 (GPIO48)

static unsigned int irqNumber;          ///< Used to share the IRQ number within this file
static bool	    ledOn = 0;          ///< Is the LED on or off? Used to invert its state (off by default)
static bool 	segOn = 0;

static int    deviceNumber;                  /// Stores the device number -- determined automatically
static char   message[256] = {0};           /// Memory for the string that is passed from userspace
static short  size_of_message;              /// Used to remember the size of the string stored
static int    numberOpens = 0;              /// Counts the number of times the device is opened
static struct class*  driverClass  = NULL; /// The struct pointer for device-driver class 
static struct device* driverDevice = NULL; /// The struct pointer for device-driver device

static DEFINE_MUTEX(driver_mutex); 
//static int result;

/// Function prototype for the custom IRQ handler function -- see below for the implementation
static irq_handler_t  ebbgpio_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs);
static irq_handler_t  ebbgpio_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs);
static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations fops =		// Function for Kernel
{
   .open = dev_open,
   .read = dev_read,
   .write = dev_write,
   .release = dev_release,
};

static int __init ebbgpio_init(void){
   int result = 0;
   printk(KERN_INFO "GPIO_TEST: Initializing the GPIO_TEST LKM.\n");
   deviceNumber = register_chrdev(0, DEVICE_NAME, &fops);
   if (deviceNumber<0){
      printk(KERN_ALERT "KDriver failed to register a Device number !!\n");
      return deviceNumber;
   }
    // To register the Device Class
   driverClass = class_create(THIS_MODULE, CLASS_NAME);
   if (IS_ERR(driverClass)){                // Checking and cleaning up if there is an error
      unregister_chrdev(deviceNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to register device class !!\n");
      return PTR_ERR(driverClass);          // Correct way to return an error on a pointer
   }
   // Register the Device Driver
   driverDevice = device_create(driverClass, NULL, MKDEV(deviceNumber, 0), NULL, DEVICE_NAME);
   if (IS_ERR(driverDevice)){               // Clean it up if there is an error
      class_destroy(driverClass);          
      unregister_chrdev(deviceNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to create the device !!\n");
      return PTR_ERR(driverDevice);
   }
   
   // Is the GPIO a valid GPIO number (e.g., the BBB has 4x32 but not all available)
   mutex_init(&driver_mutex);
  
	if (!gpio_is_valid(dig1)){
      printk(KERN_INFO "GPIO_TEST: invalid Digit 1 GPIO.\n");
      return -ENODEV;
   }
   if (!gpio_is_valid(segA)){
      printk(KERN_INFO "GPIO_TEST: invalid Segment A GPIO.\n");
      return -ENODEV;
   }
   if (!gpio_is_valid(segB)){
      printk(KERN_INFO "GPIO_TEST: invalid Segment B GPIO.\n");
      return -ENODEV;
   }
   if (!gpio_is_valid(segC)){
      printk(KERN_INFO "GPIO_TEST: invalid Segment C GPIO.\n");
      return -ENODEV;
   }
   if (!gpio_is_valid(segD)){
      printk(KERN_INFO "GPIO_TEST: invalid Segment D GPIO.\n");
      return -ENODEV;
   }
   if (!gpio_is_valid(segE)){
      printk(KERN_INFO "GPIO_TEST: invalid Segment E GPIO.\n");
      return -ENODEV;
   }
   if (!gpio_is_valid(segF)){
      printk(KERN_INFO "GPIO_TEST: invalid Segment F GPIO.\n");
      return -ENODEV;
   }
   if (!gpio_is_valid(segG)){
      printk(KERN_INFO "GPIO_TEST: invalid Segment G GPIO.\n");
      return -ENODEV;
   }
   
   // Going to set up the LED. It is a GPIO in output mode and will be on by default
   ledOn = true;
   segOn = true;
   gpio_request(dig1, "sysfs");          // gpioLED is hardcoded to 20, request it
   gpio_direction_output(dig1, ledOn);   // Set the gpio to be in output mode and on
   gpio_export(dig1, false);             // Causes gpio60 to appear in /sys/class/gpio
   
  /* gpio_request(dig2, "sysfs");          // gpioLED is hardcoded to 67, request it
   gpio_direction_output(dig2, ledOn);   // Set the gpio to be in output mode and on
   gpio_export(dig2, false);             // Causes gpio60 to appear in /sys/class/gpio
   
   gpio_request(dig3, "sysfs");          // gpioLED is hardcoded to 68, request it
   gpio_direction_output(dig3, ledOn);   // Set the gpio to be in output mode and on
   gpio_export(dig3, false);             // Causes gpio60 to appear in /sys/class/gpio
   
   gpio_request(dig4, "sysfs");          // gpioLED is hardcoded to 44, request it
   gpio_direction_output(dig4, ledOn);   // Set the gpio to be in output mode and on
   gpio_export(dig4, false);  */           // Causes gpio60 to appear in /sys/class/gpio
   
   gpio_request(segA, "sysfs");          // gpioLED is hardcoded to 48, request it
   gpio_direction_output(segA, segOn);   // Set the gpio to be in output mode and on
   gpio_export(segA, false);             // Causes gpio60 to appear in /sys/class/gpio
   
   gpio_request(segB, "sysfs");          // gpioLED is hardcoded to 49, request it
   gpio_direction_output(segB, segOn);   // Set the gpio to be in output mode and on
   gpio_export(segB, false);             // Causes gpio49 to appear in /sys/class/gpio
   
   gpio_request(segC, "sysfs");          // gpioLED is hardcoded to 69, request it
   gpio_direction_output(segC, segOn);   // Set the gpio to be in output mode and on
   gpio_export(segC, false);             // Causes gpio48 to appear in /sys/class/gpio
   
   gpio_request(segD, "sysfs");          // gpioLED is hardcoded to 117, request it
   gpio_direction_output(segD, segOn);   // Set the gpio to be in output mode and on
   gpio_export(segD, false);             // Causes gpio112 to appear in /sys/class/gpio

   gpio_request(segE, "sysfs");          // gpioLED is hardcoded to 115, request it
   gpio_direction_output(segE, segOn);   // Set the gpio to be in output mode and on
   gpio_export(segE, false);             // Causes gpio112 to appear in /sys/class/gpio
   
   gpio_request(segF, "sysfs");          // gpioLED is hardcoded to 46, request it
   gpio_direction_output(segF, segOn);   // Set the gpio to be in output mode and on
   gpio_export(segF, false);             // Causes gpio112 to appear in /sys/class/gpio
   
   gpio_request(segG, "sysfs");          // gpioLED is hardcoded to 112, request it
   gpio_direction_output(segG, segOn);   // Set the gpio to be in output mode and on
   gpio_export(segG, false);             // Causes gpio112 to appear in /sys/class/gpio
   
   gpio_request(gpioButton, "sysfs");       // Set up the gpioButton
   gpio_direction_input(gpioButton);        // Set the button GPIO to be an input
   gpio_set_debounce(gpioButton, 200);      // Debounce the button with a delay of 200ms
   gpio_export(gpioButton, false);          // Causes gpio115 to appear in /sys/class/gpio
			                  
   // This next call requests an interrupt line
   result = request_irq(irqNumber,             // The interrupt number requested
                        (irq_handler_t) ebbgpio_irq_handler, // The pointer to the handler function below
                        IRQF_TRIGGER_RISING,   // Interrupt on rising edge (button press, not release)
                        "ebb_gpio_handler",    // Used in /proc/interrupts to identify the owner
                        NULL);                 // The *dev_id for shared interrupt lines, NULL is okay

   printk(KERN_INFO "GPIO_TEST: The interrupt request result is: %d\n", result);
   return result;
}

static void __exit ebbgpio_exit(void){
	
	//Existence of the device 
   mutex_destroy(&driver_mutex);                           // destroy the mutex
   device_destroy(driverClass, MKDEV(deviceNumber, 0));    // To remove the device
   class_unregister(driverClass);                          // To unregister the device class
   class_destroy(driverClass);                             // To remove the device class
   unregister_chrdev(deviceNumber, DEVICE_NAME);
   
   printk(KERN_INFO "GPIO_TEST: The button state is currently: %d\n", gpio_get_value(gpioButton));
 
   gpio_set_value(segA, 0);	// Turn the Segment A off, makes it clear the device was unloaded
   gpio_unexport(segA);           // Unexport the Segment A
   
   gpio_set_value(segB, 0);          // Turn the Segment B off, makes it clear the device was unloaded
   gpio_unexport(segB);              // Unexport the Segment B
   
   gpio_set_value(segC, 0);        // Turn the Segment C off, makes it clear the device was unloaded
   gpio_unexport(segC);            // Unexport the Segment C
   
   gpio_set_value(segD, 0);        // Turn the Segment D off, makes it clear the device was unloaded
   gpio_unexport(segD);		// Unexport the Segment D
   
   gpio_set_value(segE, 0);        // Turn the Segment E off, makes it clear the device was unloaded
   gpio_unexport(segE);		// Unexport the Segment E
   
   gpio_set_value(segF, 0);        // Turn the Segment F off, makes it clear the device was unloaded
   gpio_unexport(segF);		// Unexport the Segment F
   
   gpio_set_value(segG, 0);        // Turn the Segment G off, makes it clear the device was unloaded
   gpio_unexport(segG);		// Unexport the Segment G
   
   free_irq(irqNumber, NULL);           // Free the IRQ number, no *dev_id required in this case
   gpio_unexport(gpioButton);           // Unexport the Button GPIO
   
   gpio_free(segA);               	// Free the Segment A GPIO
   gpio_free(segB);               	// Free the Segment B GPIO
   gpio_free(segC);                	// Free the Segment C GPIO
   gpio_free(segD);                	// Free the Segment D GPIO
   gpio_free(segE);                	// Free the Segment E GPIO
   gpio_free(segF);                	// Free the Segment F GPIO
   gpio_free(segG);                	// Free theSegment G GPIO
   gpio_free(gpioButton);           // Free the Button GPIO
   
printk(KERN_INFO "GPIO_TEST: Goodbye from the LKM!\n");
}

static irq_handler_t ebbgpio_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs){
   ledOn = !ledOn;                          	// Invert the LED state on each button press
   gpio_set_value(dig1, ledOn);          	// Set the physical Digit 1 accordingly
   //gpio_set_value(dig2, ledOn);          	// Set the physical Digit 2 accordingly
   //gpio_set_value(dig3, ledOn);          	// Set the physical Digit 3 accordingly
   //gpio_set_value(dig4, ledOn);          	// Set the physical Digit 4 accordingly
   gpio_set_value(segA, segOn);          	// Set the physical Segment A accordingly
   gpio_set_value(segB, segOn);          	// Set the physical Segment B accordingly
   gpio_set_value(segC, segOn);          	// Set the physical Segment C accordingly
   gpio_set_value(segD, segOn);          	// Set the physical Segment D accordingly
   gpio_set_value(segE, segOn);          	// Set the physical Segment E accordingly
   gpio_set_value(segF, segOn);          	// Set the physical Segment F accordingly
   gpio_set_value(segG, segOn);	        	// Set the physical Segment G accordingly
   printk(KERN_INFO "GPIO_TEST: Interrupt! (button state is %d)\n", gpio_get_value(gpioButton));
   return (irq_handler_t) IRQ_HANDLED;      	// Announce that the IRQ has been handled correctly
}

//Function to Open the device
static int dev_open(struct inode *inodep, struct file *filep){
   numberOpens++;
   printk(KERN_INFO "KDriver: Device has been opened %d time(s)\n", numberOpens);

   mutex_lock(&driver_mutex);                             //lock the mutex

   return 0;
}
// Function for Releasing the device
static int dev_release(struct inode *inodep, struct file *filep){

   mutex_unlock(&driver_mutex);                      //unlock the mutex

   printk(KERN_INFO "KDriver: Device successfully closed\n");
   return 0;
}

//Function to Read to the device
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
   int error_count = 0;
   // copy_to_user has the format ( * to, *from, size) and returns 0 on success
   error_count = copy_to_user(buffer, message, size_of_message);

   if (error_count==0){            	// Success, if true.
      printk(KERN_INFO "KDriver: Sent %d characters to the user\n", size_of_message);
      return (size_of_message=0);  	// clear the position to the start and return 0
   }
   else {
      printk(KERN_INFO "KDriver: Failed to send %d characters to the user\n", error_count);
      return -EFAULT;             	// Returning -14 as bad address message
   }
}

//Function to Turn ON and Turn OFF the LEDs
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){

   ledOn=true;
   segOn=true;
   //int i;
   sprintf(message, "%s(%zu letters)", buffer, len);   	// Appending the received string with its length
   //for(i=0;i<len;i++){
	gpio_direction_output(dig1, ledOn);	// Turn the Digit1 on, makes it clear the device was loaded
   	gpio_export(dig1, false);
	/*gpio_direction_output(dig2, ledOn);	// Turn the Digit2 on, makes it clear the device was loaded
   	gpio_export(dig2, false);
	gpio_direction_output(dig3, ledOn);	// Turn the Digit3 on, makes it clear the device was loaded
   	gpio_export(dig3, false);
	gpio_direction_output(dig4, ledOn);	// Turn the Digit4 on, makes it clear the device was loaded
   	gpio_export(dig4, false);*/

	if (message[0]=='0'){				//switching off Segment G 
	gpio_set_value(segB, 0);
	gpio_unexport(segB);
	gpio_set_value(segC, 0);
	gpio_unexport(segC);
	gpio_set_value(segD, 0);
	gpio_unexport(segD);
	gpio_set_value(segE, 0);
	gpio_unexport(segE);
	gpio_set_value(segF, 0);
	gpio_unexport(segF);
	gpio_direction_output(segG, segOn);	
   	gpio_export(segG, false);
   }
   else if (message[0]=='1'){			//Switching off Segments A,D,E,F,G
	gpio_direction_output(segA, segOn);	
   	gpio_export(segA, false);gpio_set_value(segB, 0);
	gpio_unexport(segB);
	gpio_set_value(segC, 0);
	gpio_unexport(segC);
	gpio_direction_output(segD, segOn);	
   	gpio_export(segD, false);
	gpio_direction_output(segE, segOn);	
   	gpio_export(segE, false);
	gpio_direction_output(segF, segOn);
	gpio_direction_output(segG, segOn);	
   	gpio_export(segG, false);
	
   }
   else if (message[0]=='2'){			//Switching off Segments c AND F
	gpio_set_value(segA, 0);
	gpio_unexport(segA);
	gpio_set_value(segB, 0);
	gpio_unexport(segB);
	gpio_direction_output(segC, segOn);	
   	gpio_export(segC, false);
	gpio_set_value(segD, 0);
	gpio_unexport(segD);
	gpio_set_value(segE, 0);
	gpio_unexport(segE);
	gpio_direction_output(segF, segOn);	
   	gpio_export(segF, false);
	gpio_set_value(segG, 0);
	gpio_unexport(segG);
		
   }
   else if (message[0]=='3'){			//Turning on Segments A,B,C,D,G
   	gpio_set_value(segA, 0);
	gpio_unexport(segA);
	gpio_set_value(segB, 0);
	gpio_unexport(segB);
	gpio_set_value(segC, 0);
	gpio_unexport(segC);
	gpio_set_value(segD, 0);
	gpio_unexport(segD);
	gpio_direction_output(segE, segOn);	
   	gpio_export(segE, false);
	gpio_direction_output(segF, segOn);	
   	gpio_export(segF, false);
	gpio_set_value(segG, 0);
	gpio_unexport(segG);
   }
   else if (message[0]=='4'){
	gpio_direction_output(segA, segOn);	//Turning on Segments B,C,F,G
   	gpio_export(segA, false);
	gpio_set_value(segB, 0);
	gpio_unexport(segB);
	gpio_set_value(segC, 0);
	gpio_unexport(segC);
	gpio_direction_output(segD, segOn);	
   	gpio_export(segD, false);
	gpio_direction_output(segE, segOn);	
   	gpio_export(segE, false);
	gpio_set_value(segF, 0);
	gpio_unexport(segF);
	gpio_set_value(segG, 0);
	gpio_unexport(segG);
   }
   else if (message[0]=='5'){				//Turning on Segments A,C,D,F,G
   	gpio_set_value(segA, 0);
	gpio_unexport(segA);
	gpio_direction_output(segB, segOn);	
   	gpio_export(segB, false);
	gpio_set_value(segC, 0);
	gpio_unexport(segC);
	gpio_set_value(segD, 0);
	gpio_unexport(segD);
	gpio_direction_output(segE, segOn);	
   	gpio_export(segE, false);
	gpio_set_value(segF, 0);
	gpio_unexport(segF);
	gpio_set_value(segG, 0);
	gpio_unexport(segG);
   }
   else if (message[0]=='6'){				//Turning on Segments A,C,D,E,F,G
   	gpio_set_value(segA, 0);
	gpio_unexport(segA);
	gpio_direction_output(segB, segOn);	
   	gpio_export(segB, false);
	gpio_set_value(segC, 0);
	gpio_unexport(segC);
	gpio_set_value(segD, 0);
	gpio_unexport(segD);
	gpio_set_value(segE, 0);
	gpio_unexport(segE);
	gpio_set_value(segF, 0);
	gpio_unexport(segF);
	gpio_set_value(segG, 0);
	gpio_unexport(segG);
   }
   else if (message[0]=='7'){			//Turning on Segments A,B,C
   	gpio_set_value(segA, 0);
	gpio_unexport(segA);
	gpio_set_value(segB, 0);
	gpio_unexport(segB);
	gpio_set_value(segC, 0);
	gpio_unexport(segC);
	gpio_set_value(segD, 0);
	gpio_unexport(segD);
	gpio_direction_output(segE, segOn);	
   	gpio_export(segE, false);
	gpio_direction_output(segF, segOn);	
   	gpio_export(segF, false);
	gpio_direction_output(segG, segOn);	
   	gpio_export(segG, false);
   }
   else if (message[0]=='8'){				//Turning on Segments A,B,C,D,E,F,G
   	gpio_set_value(segA, 0);
	gpio_unexport(segA);
	gpio_set_value(segB, 0);
	gpio_unexport(segB);
	gpio_set_value(segC, 0);
	gpio_unexport(segC);
	gpio_set_value(segD, 0);
	gpio_unexport(segD);
	gpio_set_value(segE, 0);
	gpio_unexport(segE);
	gpio_set_value(segF, 0);
	gpio_unexport(segF);
	gpio_set_value(segG, 0);
	gpio_unexport(segG);	
   }
   else if (message[0]=='9'){				//Turning on Segments A,B,C,D,F,G
   	gpio_set_value(segA, 0);
	gpio_unexport(segA);
	gpio_set_value(segB, 0);
	gpio_unexport(segB);
	gpio_set_value(segC, 0);
	gpio_unexport(segC);
	gpio_set_value(segD, 0);
	gpio_unexport(segD);
	gpio_direction_output(segE, segOn);	
   	gpio_export(segE, false);
	gpio_set_value(segF, 0);
	gpio_unexport(segF);
	gpio_set_value(segG, 0);
	gpio_unexport(segG);
   }
	
   //} //for loop end
   size_of_message = strlen(message);                 	// store the length of the stored message
   printk(KERN_INFO "KDriver: Received %zu characters from the user.\n", len);
   return len;
}

/// This next calls are  mandatory -- they identify the initialization function
/// and the cleanup function (as above).

module_init(ebbgpio_init);
module_exit(ebbgpio_exit);
