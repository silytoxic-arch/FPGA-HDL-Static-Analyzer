entity vhdl_process_no_suspension is
  port (a : in std_logic; q : out std_logic);
end entity;

architecture rtl of vhdl_process_no_suspension is begin
  no_suspend : process begin
    q <= a;
  end process;
end architecture;
