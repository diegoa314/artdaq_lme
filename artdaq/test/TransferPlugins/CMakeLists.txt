
# This is an integration test that exercises SHandles and RHandles,
# making sure they can communicate as expected.
#cet_test(transfer_driver_t HANDBUILT
#  TEST_EXEC mpirun
#  TEST_ARGS -hosts localhost -np 5 transfer_driver_mpi transfer_driver_mpi.fcl
#  DATAFILES fcl/transfer_driver_mpi.fcl
#  TEST_PROPERTIES RUN_SERIAL 1
#  )

#cet_test(transfer_driver_validate_t HANDBUILT
#  TEST_EXEC mpirun
#  TEST_ARGS -hosts localhost -np 5 transfer_driver_mpi transfer_driver_mpi_validate.fcl
#  DATAFILES fcl/transfer_driver_mpi_validate.fcl
#  TEST_PROPERTIES RUN_SERIAL 1
#  )

 art_make_exec(NAME transfer_driver # NO_INSTALL -- comment out to install
SOURCE
transfer_driver.cc
   LIBRARIES
  artdaq_DAQrate
   artdaq_DAQdata
artdaq_TransferPlugins
  ${FHICLCPP}
   )

  cet_script(runTransferTest.sh)

  cet_test(transfer_driver_tcp_t HANDBUILT
	TEST_EXEC runTransferTest.sh
	TEST_ARGS transfer_driver_tcp.fcl 2
	DATAFILES fcl/transfer_driver_tcp.fcl
  #TEST_PROPERTIES RUN_SERIAL 1
	)

  cet_test(transfer_driver_shmem_t HANDBUILT
	TEST_EXEC runTransferTest.sh
	TEST_ARGS transfer_driver_shmem.fcl 2
	DATAFILES fcl/transfer_driver_shmem.fcl
  #TEST_PROPERTIES RUN_SERIAL 1
	)

  cet_test(transfer_driver_tcp_validate_t HANDBUILT
	TEST_EXEC runTransferTest.sh
	TEST_ARGS transfer_driver_tcp_validate.fcl 2
	DATAFILES fcl/transfer_driver_tcp_validate.fcl
  #TEST_PROPERTIES RUN_SERIAL 1
	)

  cet_test(transfer_driver_shmem_validate_t HANDBUILT
	TEST_EXEC runTransferTest.sh
	TEST_ARGS transfer_driver_shmem_validate.fcl 2
	DATAFILES fcl/transfer_driver_shmem_validate.fcl
  #TEST_PROPERTIES RUN_SERIAL 1
	)

  cet_test(transfer_driver_tcp_multi HANDBUILT
      TEST_EXEC runTransferTest.sh
      TEST_ARGS transfer_driver_tcp_multi.fcl 2
      DATAFILES fcl/transfer_driver_tcp_multi.fcl
  #TEST_PROPERTIES RUN_SERIAL 1
      )

 # cet_test(transfer_driver_shmem_multi HANDBUILT
 #     TEST_EXEC runTransferTest.sh
 #     TEST_ARGS transfer_driver_shmem_multi.fcl 2
 #     DATAFILES fcl/transfer_driver_shmem_multi.fcl
 # #TEST_PROPERTIES RUN_SERIAL 1
 #     )

  cet_test(transfer_driver_tcp_complex HANDBUILT
      TEST_EXEC runTransferTest.sh
      TEST_ARGS transfer_driver_tcp_complex.fcl 4
      DATAFILES fcl/transfer_driver_tcp_complex.fcl
  #TEST_PROPERTIES RUN_SERIAL 1
      )

  cet_test(transfer_driver_shmem_complex HANDBUILT
      TEST_EXEC runTransferTest.sh
      TEST_ARGS transfer_driver_shmem_complex.fcl 4
      DATAFILES fcl/transfer_driver_shmem_complex.fcl
  #TEST_PROPERTIES RUN_SERIAL 1
      )

  cet_test(transfer_driver_tcp_empty_t HANDBUILT
	TEST_EXEC runTransferTest.sh
	TEST_ARGS transfer_driver_tcp_empty.fcl 2
	DATAFILES fcl/transfer_driver_tcp_empty.fcl
  #TEST_PROPERTIES RUN_SERIAL 1
	)

  cet_test(transfer_driver_shmem_empty_t HANDBUILT
	TEST_EXEC runTransferTest.sh
	TEST_ARGS transfer_driver_shmem_empty.fcl 2
	DATAFILES fcl/transfer_driver_shmem_empty.fcl
  #TEST_PROPERTIES RUN_SERIAL 1
	)


  cet_test(transfer_driver_tcp_nonblocking_t HANDBUILT
	TEST_EXEC runTransferTest.sh
	TEST_ARGS transfer_driver_tcp_nonblocking.fcl 2
	DATAFILES fcl/transfer_driver_tcp_nonblocking.fcl
  #TEST_PROPERTIES RUN_SERIAL 1
	)

  cet_test(transfer_driver_shmem_nonblocking_t HANDBUILT
	TEST_EXEC runTransferTest.sh
	TEST_ARGS transfer_driver_shmem_nonblocking.fcl 2
	DATAFILES fcl/transfer_driver_shmem_nonblocking.fcl
  #TEST_PROPERTIES RUN_SERIAL 1
	)

  cet_test(transfer_driver_tcp_nonblocking_broadcast_t HANDBUILT
	TEST_EXEC runTransferTest.sh
	TEST_ARGS transfer_driver_tcp_nonblocking_broadcast.fcl 4
	DATAFILES fcl/transfer_driver_tcp_nonblocking_broadcast.fcl
  #TEST_PROPERTIES RUN_SERIAL 1
	)

  cet_test(transfer_driver_shmem_nonblocking_broadcast_t HANDBUILT
	TEST_EXEC runTransferTest.sh
	TEST_ARGS transfer_driver_shmem_nonblocking_broadcast.fcl 4
	DATAFILES fcl/transfer_driver_shmem_nonblocking_broadcast.fcl
  #TEST_PROPERTIES RUN_SERIAL 1
	)

  cet_test(transfer_driver_tcp_broadcast_t HANDBUILT
	TEST_EXEC runTransferTest.sh
	TEST_ARGS transfer_driver_tcp_broadcast.fcl 4
	DATAFILES fcl/transfer_driver_tcp_broadcast.fcl
  #TEST_PROPERTIES RUN_SERIAL 1
	)

  cet_test(transfer_driver_shmem_broadcast_t HANDBUILT
	TEST_EXEC runTransferTest.sh
	TEST_ARGS transfer_driver_shmem_broadcast.fcl 4
	DATAFILES fcl/transfer_driver_shmem_broadcast.fcl
  #TEST_PROPERTIES RUN_SERIAL 1
	)
