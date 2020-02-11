#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <modbus/modbus-rtu.h>
 
int main()
{
        modbus_t *ctx;
 
        // libmodbus rtu ���ؽ�Ʈ ����
        ctx = modbus_new_rtu("/dev/ttyUSB0", 9600, 'N', 8, 1);
        /*
         * ����̽� : /dev/ttyXXX
         * ���ۼӵ� : 9600, 19200, 57600, 115200
         * �и�Ƽ��� : N (none) / E (even) / O (odd)
         * ������ ��Ʈ : 5,6,7 or 8
         * ���� ��Ʈ : 1, 2

        */
        if (ctx == NULL) {
                fprintf(stderr, "Unable to create the libmodbus context\n");
                return -1;
        }
 
        modbus_set_slave(ctx, 2); // �����̺� �ּ�
        modbus_set_debug(ctx,FALSE); // ����� Ȱ��
 
        // ����
        if (modbus_connect(ctx) == -1) {
                fprintf(stderr, "Unable to connect %s\n", modbus_strerror(errno));
                modbus_free(ctx);
                return -1;
        }
 
        uint16_t *read_registers;
        read_registers = (uint16_t *) malloc(256 * sizeof(uint16_t));
		//���� �µ� 25.5 �� ���� 
		modbus_write_register(ctx, 211, 255);  
		
		while(1)
	{
		printf("request ---------------------\n"); // 2545 => 25.45 C
		// 0x03 ��� �ڵ� (read holding register)
        // 0x012C : �ּ� / 2 : ���� ���� (����)
        // ����µ� modbus_read_registers(ctx, 200, 1, read_registers)
		// �����µ� modbus_read_registers(ctx, 211, 1, read_registers)
		if( modbus_read_registers(ctx, 200, 1, read_registers) != -1 )
		{
		   // print temperature / humidity
			printf("cur temp  %d \n",  read_registers[0]); // 2545 => 25.45 C

		} else {
			printf("read error \n"); // 2545 => 25.45 C

		}

		sleep(1) ;
		if( modbus_read_registers(ctx, 211, 1, read_registers) != -1 )
		{
		   // print temperature / humidity
			printf("set temp  %d \n",  read_registers[0]); // 2545 => 25.45 C

		} else {
			printf("read error \n"); // 2545 => 25.45 C

		}

 
 
		sleep(1) ;

	}
 
   /*    
		// 0x06 ��� �ڵ� (Preset Single Register)
        // 0x0000 : �ּ�(�����̺� �ּ� ����) / 245 : ���� 242���� 245�� ����
        modbus_write_register(ctx, 0x0000, 245);
 
        // ����� �����̺� �ּҷ� �翬��
        modbus_close(ctx);
        modbus_set_slave(ctx, 245);
        if (modbus_connect(ctx) == -1) {
                fprintf(stderr, "Unable to connect %s\n", modbus_strerror(errno));
                modbus_free(ctx);
                return -1;
        }
 
        // 0x04 ��� �ڵ� (Read Input Registers)
        // ������ �����̺� �ּҰ��� �б�
        modbus_read_input_registers(ctx, 0x0000, 1, read_registers);
        printf("Slave Address : %d\n", read_registers[0]);
 */
        modbus_close(ctx);
        modbus_free(ctx);
        return 0;
}
