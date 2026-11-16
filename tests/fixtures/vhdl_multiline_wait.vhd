entity vhdl_multiline_wait is
  port (a : in std_logic; q : out std_logic);
end entity;
architecture rtl of vhdl_multiline_wait is begin
  worker : process begin
    wait
      until a = '1';
    q <= a;
  end process;
end architecture;
