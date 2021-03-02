

file FLASH_RUN/blue/blue.elf

target remote localhost:3333

define send_fc
 #set test_label = 0xfc
 #set test_buf[0] = 0x64
 #set test_len = 1
 #call gdm_send_pkt (test_label, test_buf, test_len)
 call test()

 #set test_len = 10
 #call gdm_recv_pkt(&test_label, test_label, &test_len)

end
document send_fc
 sometresxt
end
