bad = 0x100;
mvme_set_am(myvme, MVME_AM_A32_ND);
for(i=0;i<8;i++)
{
bad = bad + 0x100;
// printf("SCRATCH::::%x\n", aad);  
mvme_write_value(myvme, 0xCCCC0000, 0x1200, bad);
aad = mvme_read_value(myvme,0xCCCC0000, 0x1200);
printf("SCRATCH::::%x\n", aad);  
}

bad = 0x100;
mvme_set_am(myvme, MVME_AM_A32_ND);
for(i=0;i<8;i++)
{
bad = bad + 0x100;
// printf("SCRATCH::::%x\n", aad);  
// mvme_write_value(myvme, 0xCCCC0000, 0x1200, bad);
v1720_RegisterWrite(myvme, 0xCCCC0000, 0x1200, bad);
// aad = mvme_read_value(myvme,0xCCCC0000, 0x1200);
aad = v1720_RegisterRead(myvme,0xCCCC0000, 0x1200);
// aad = v1720_RegisterRead(myvme, V1720_BASE_ADDR, 0x811C);
printf("SCRATCH::::%x\n", aad);  
}
