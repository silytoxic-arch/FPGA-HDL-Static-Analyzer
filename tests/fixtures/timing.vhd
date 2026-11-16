entity timing_fixture is
  port (clk : in std_logic; reset_async : in std_logic; sync_reset : in std_logic;
        d : in std_logic; q_async : out std_logic; q_sync : out std_logic);
end entity;
architecture rtl of timing_fixture is begin
  async_register : process(clk, reset_async) begin
    if reset_async = '0' then q_async <= '0';
    elsif rising_edge(clk) then q_async <= d; end if;
  end process;
  sync_register : process(clk) begin
    if rising_edge(clk) then
      if sync_reset = '1' then q_sync <= '0'; else q_sync <= d; end if;
    end if;
  end process;
end architecture;
