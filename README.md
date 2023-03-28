# cache_readdir_stat

A closed source software we use invokes file system related library calls
a million times.

This performance bug cannot be fixed.

The shared library cache_readdir_stat.so will be used with
   LD_PRELOAD=<path to cache_readdir_stat.so>

It will keep the result of these  calls in a dictionary
such that it is invoked only once per file.



 Calling  opendir /local/wine_prefix/wine_for_diann/dosdevices/z:/home/cgille/tmp/ZIPsFS/mnt/PRO1/Data/30-0046/20230126_PRO1_KTT_002_30-0046_LisaKahl_P01_VNATSerAuxpM1evoM2Glucose10mMGlycine3mM_dia_BC7_1_12094.d
  Calling  opendir /local/wine_prefix/wine_for_diann/dosdevices/z:/home/cgille/tmp/ZIPsFS/mnt/PRO1/Data/30-0046/20230126_PRO1_KTT_002_30-0046_LisaKahl_P01_VNATSerAuxpM1evoM2Glucose10mMGlycine3mM_dia_BC7_1_12094.d
   Calling fstatfs 35
    Calling  opendir /local/wine_prefix/wine_for_diann/dosdevices/z:/home/cgille/tmp/ZIPsFS/mnt/PRO1/Data/30-0046/20230126_PRO1_KTT_002_30-0046_LisaKahl_P01_VNATSerAuxpM1evoM2Glucose10mMGlycine3mM_dia_BC7_1_12094.d
     Calling fstatfs 30
      Calling  opendir /local/wine_prefix/wine_for_diann/dosdevices/z:/home/cgille/tmp/ZIPsFS/mnt/PRO1/Data/30-0046/20230126_PRO1_KTT_002_30-0046_LisaKahl_P01_VNATSerAuxpM1evoM2Glucose10mMGlycine3mM_dia_BC7_1_12094.d
       Calling fstatfs 33
        Calling  opendir /local/wine_prefix/wine_for_diann/dosdevices/z:/home/cgille/tmp/ZIPsFS/mnt/PRO1/Data/30-0046/20230126_PRO1_KTT_002_30-0046_LisaKahl_P01_VNATSerAuxpM1evoM2Glucose10mMGlycine3mM_dia_BC7_1_12094.d
         Calling fstatfs 30
          Calling  opendir /local/wine_prefix/wine_for_diann/dosdevices/z:/home/cgille/tmp/ZIPsFS/mnt/PRO1/Data/30-0046/20230126_PRO1_KTT_002_30-0046_LisaKahl_P01_VNATSerAuxpM1evoM2Glucose10mMGlycine3mM_dia_BC7_1_12094.d
           Calling fstatfs 33
            Calling fstatfs 35
             Calling  opendir /local/wine_prefix/wine_for_diann/dosdevices/z:/home/cgille/tmp/ZIPsFS/mnt/PRO1/Data/30-0046/20230126_PRO1_KTT_002_30-0046_LisaKahl_P01_VNATSerAuxpM1evoM2Glucose10mMGlycine3mM_dia_BC7_1_12094.d
              Calling  opendir /local/wine_prefix/wine_for_diann/dosdevices/z:/home/cgille/tmp/ZIPsFS/mnt/PRO1/Data/30-0046/20230126_PRO1_KTT_002_30-0046_LisaKahl_P01_VNATSerAuxpM1evoM2Glucose10mMGlycine3mM_dia_BC7_1_12094.d
               Calling fstatfs 30
                Calling  opendir /local/wine_prefix/wine_for_diann/dosdevices/z:/home/cgille/tmp/ZIPsFS/mnt/PRO1/Data/30-0046/20230126_PRO1_KTT_002_30-0046_LisaKahl_P01_VNATSerAuxpM1evoM2Glucose10mMGlycine3mM_dia_BC7_1_12094.d
                 Calling fstatfs 30
                  Calling  opendir /local/wine_prefix/wine_for_diann/dosdevices/z:/home/cgille/tmp/ZIPsFS/mnt/PRO1/Data/30-0046/20230126_PRO1_KTT_002_30-0046_LisaKahl_P01_VNATSerAuxpM1evoM2Glucose10mMGlycine3mM_dia_BC7_1_12094.d
                   Calling fstatfs 33
                    Calling  opendir /local/wine_prefix/wine_for_diann/dosdevices/z:/home/cgille/tmp/ZIPsFS/mnt/PRO1/Data/30-0046/20230126_PRO1_KTT_002_30-0046_LisaKahl_P01_VNATSerAuxpM1evoM2Glucose10mMGlycine3mM_dia_BC7_1_12094.d
                     Calling fstatfs 35
                      Calling  opendir /local/wine_prefix/wine_for_diann/dosdevices/z:/home/cgille/tmp/ZIPsFS/mnt/PRO1/Data/30-0046/20230126_PRO1_KTT_002_30-0046_LisaKahl_P01_VNATSerAuxpM1evoM2Glucose10mMGlycine3mM_dia_BC7_1_12094.d
                       Calling fstatfs 33
                        Calling  opendir /local/wine_prefix/wine_for_diann/dosdevices/z:/home/cgille/tmp/ZIPsFS/mnt/PRO1/Data/30-0046/20230126_PRO1_KTT_002_30-0046_LisaKahl_P01_VNATSerAuxpM1evoM2Glucose10mMGlycine3mM_dia_BC7_1_12094.d
                         Calling fstatfs 30
                          Calling  opendir /local/wine_prefix/wine_for_diann/dosdevices/z:/home/cgille/tmp/ZIPsFS/mnt/PRO1/Data/30-0046/20230126_PRO1_KTT_002_30-0046_LisaKahl_P01_VNATSerAuxpM1evoM2Glucose10mMGlycine3mM_dia_BC7_1_12094.
