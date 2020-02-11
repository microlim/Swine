#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <modbus/modbus-rtu.h>
 
int main()
{
        modbus_t *ctx;
 
        // libmodbus rtu 컨텍스트 생성
        ctx = modbus_new_rtu("/dev/ttyUSB0", 9600, 'N', 8, 1);
        /*
         * 디바이스 : /dev/ttyXXX
         * 전송속도 : 9600, 19200, 57600, 115200
         * 패리티모드 : N (none) / E (even) / O (odd)
         * 데이터 비트 : 5,6,7 or 8
         * 스톱 비트 : 1, 2

        */
        if (ctx == NULL) {
                fprintf(stderr, "Unable to create the libmodbus context\n");
                return -1;
        }
 
        modbus_set_slave(ctx, 2); // 슬레이브 주소
        modbus_set_debug(ctx,FALSE); // 디버깅 활성
 
        // 연결
        if (modbus_connect(ctx) == -1) {
                fprintf(stderr, "Unable to connect %s\n", modbus_strerror(errno));
                modbus_free(ctx);
                return -1;
        }
 
        uint16_t *read_registers;
        read_registers = (uint16_t *) malloc(256 * sizeof(uint16_t));
		//설정 온도 25.5 로 설정 
		modbus_write_register(ctx, 211, 255);  
		
		while(1)
	{
		printf("request ---------------------\n"); // 2545 => 25.45 C
		// 0x03 기능 코드 (read holding register)
        // 0x012C : 주소 / 2 : 읽을 갯수 (예시)
        // 현재온도 modbus_read_registers(ctx, 200, 1, read_registers)
		// 설정온도 modbus_read_registers(ctx, 211, 1, read_registers)
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
		// 0x06 기능 코드 (Preset Single Register)
        // 0x0000 : 주소(슬레이브 주소 변경) / 245 : 기존 242에서 245로 수정
        modbus_write_register(ctx, 0x0000, 245);
 
        // 변경된 슬레이브 주소로 재연결
        modbus_close(ctx);
        modbus_set_slave(ctx, 245);
        if (modbus_connect(ctx) == -1) {
                fprintf(stderr, "Unable to connect %s\n", modbus_strerror(errno));
                modbus_free(ctx);
                return -1;
        }
 
        // 0x04 기능 코드 (Read Input Registers)
        // 설정된 슬레이브 주소값을 읽기
        modbus_read_input_registers(ctx, 0x0000, 1, read_registers);
        printf("Slave Address : %d\n", read_registers[0]);
 */
        modbus_close(ctx);
        modbus_free(ctx);
        return 0;
}
