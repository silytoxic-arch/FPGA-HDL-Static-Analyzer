entity vhdl_wait_with_sensitivity is
  port (a : in std_logic; q : out std_logic);
end entity;

architecture rtl of vhdl_wait_with_sensitivity is begin
  illegal_wait : process(a) begin
    wait on a;
    q <= a;
  end process;
end architecture;
