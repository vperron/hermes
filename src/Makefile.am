if DEBUG_ENABLED
AM_CFLAGS = -Wall -g -ggdb -O2 -Wno-unused-variable -Wno-pointer-to-int-cast -DDEBUG $(zeromq_CFLAGS)
else
AM_CFLAGS = -Wall -Os -s -Wno-unused-variable -DNDEBUG $(zeromq_CFLAGS)
endif

bin_PROGRAMS = hermes hermes_unittest hermes_e2etest box-status

generic_sources = config.c zeromq.c ftp.c sops.c superfasthash.c utils.c throughput_agent.c \
									ftp_agent.c http_agent.c hashtable_agent.c persist_agent.c failure_agent.c

hermes_SOURCES = main.c $(generic_sources)

hermes_unittest_CFLAGS = $(AM_CFLAGS) -DTESTING -Wno-pointer-to-int-cast -Wno-unused-function
hermes_unittest_SOURCES = hermes_unittest.c

hermes_e2etest_CFLAGS = $(AM_CFLAGS) -DE2E_TESTING -Wno-pointer-to-int-cast -Wno-unused-function
hermes_e2etest_SOURCES = hermes_e2etest.c http_agent.c hashtable_agent.c failure_agent.c \
												 superfasthash.c throughput_agent.c

box_status_SOURCES = box-status.c config.c sops.c

TESTS = hermes_unittest hermes_e2etest
