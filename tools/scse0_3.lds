OUTPUT_ARCH(riscv)
ENTRY(_start_mos)
BASE_ADDRESS = 0x80200000;

SECTIONS
{
  /*. = 0x78000080;
  start_exc_vec = .;
  .exc_vec : {
        *(.text.exc_vec)
  }*/

  . = BASE_ADDRESS;

  start_text = . ;
  .text : {
	*(.text.start_mos)
        *(.text)
    }

  . = ALIGN(4K);
  start_exc_vec = . ;
  .exc_vec : {
	*(.text.exc_vec)
    }

  . = ALIGN(4K);
  end_text = . ;
  start_bss = . ;
  .bss  : {
   *(.bss)
   }

  . = ALIGN(4K);
  end_bss = . ;
  start_data = . ;
  .data : {
        *(.data)
        }

  .sdata : {
    *(.sdata)
  }

  . = ALIGN(4K);
  end_data = . ;
  start_kern_stk = . ;
  .kern_stk : {
        *(.data.stk)
  }

  /*. = 0x80600000;*/
  . = ALIGN(4K);
  end_kern_stk = . ;
  end = . ;
}
