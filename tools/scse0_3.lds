OUTPUT_ARCH(riscv)
ENTRY(_start_mos)
BASE_ADDRESS = 0x80200000;

SECTIONS
{
  /*. = 0x80000080;
  .except_vec3 : {
        *(.text.exc_vec3)
  }*/

  . = BASE_ADDRESS;

  start_text = . ;
  .text : {
	*(.text ._start_mos)
        *(.text)
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
