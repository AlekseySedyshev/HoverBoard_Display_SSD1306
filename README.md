CPU - CH32V003J4M6  Small Risc-V  8pins MCU

LCD - SSD1306

Video: https://youtu.be/FTMUV9VQzVY

At the first, it's was writen and tested on that project: https://github.com/larsmm/hoverboard-firmware-hack-FOC-bbcar
For connect to hoverboard this display you need 
1) Find in the project file: SRC\mine.c,
2) Open file and find next strings:

//---------------------------------------------

#else
          // printf("in1:%i in2:%i cmdL:%i cmdR:%i BatADC:%i BatV:%i TempADC:%i Temp:%i velL:%i velR:%i curL:%i curR:%i\r\n",
          printf("in1:%i in2:%i cmdL:%i cmdR:%i BatADC:%i BatV:%i TempADC:%i Temp:%i velL:%i velR:%i\r\n",
            input1[inIdx].raw,        // 1: INPUT1
            input2[inIdx].raw,        // 2: INPUT2
            cmdL,                     // 3: output command: [-1000, 1000]
            cmdR,                     // 4: output command: [-1000, 1000]
            adc_buffer.batt1,         // 5: for battery voltage calibration
            batVoltageCalib,          // 6: for verifying battery voltage calibration
            board_temp_adcFilt,       // 7: for board temperature calibration
            board_temp_deg_c,         // 8: for verifying board temperature calibration
            rtY_Left.n_mot,           // 9: motor speed
            rtY_Right.n_mot           //10: motor speed
            // rtY_Left.iq,              //11: motor q axis current
            // rtY_Right.iq              //12: motor q axis current
          );
          
//---------------------------------------------

Change it on:

//---------------------------------------------------------------------------------------------------------------

#else
          printf("in1:%i in2:%i cmdL:%i cmdR:%i BatADC:%i BatV:%i TempADC:%i Temp:%i velL:%i velR:%i curL:%i curR:%i\r\n",
          //printf("in1:%i in2:%i cmdL:%i cmdR:%i BatADC:%i BatV:%i TempADC:%i Temp:%i velL:%i velR:%i\r\n",
            input1[inIdx].raw,        // 1: INPUT1
            input2[inIdx].raw,        // 2: INPUT2
            cmdL,                     // 3: output command: [-1000, 1000]
            cmdR,                     // 4: output command: [-1000, 1000]
            adc_buffer.batt1,         // 5: for battery voltage calibration
            batVoltageCalib,          // 6: for verifying battery voltage calibration
            board_temp_adcFilt,       // 7: for board temperature calibration
            board_temp_deg_c,         // 8: for verifying board temperature calibration
            rtY_Left.n_mot,           // 9: motor speed
            rtY_Right.n_mot,           //10: motor speed
            left_dc_curr,              //11: Left motor  current
            right_dc_curr              //12: Right motor current
          );
          
//----------------------------------------------------------------------------------------------------------------

Save the project, compile and upload it in the your hoverboard
