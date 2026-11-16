entity async_reset_not_first is
  port (
    clk     : in  bit;
    rst_n   : in  bit;
    enable  : in  bit;
    d       : in  bit;
    q_bad   : out bit;
    q_good  : out bit
  );
end entity;

architecture rtl of async_reset_not_first is
begin
  masked_reset : process(clk, rst_n)
  begin
    if enable = '1' then
      q_bad <= d;
    elsif rst_n = '0' then
      q_bad <= '0';
    elsif rising_edge(clk) then
      q_bad <= d;
    end if;
  end process;

  valid_reset : process(clk, rst_n)
  begin
    if rst_n = '0' then
      q_good <= '0';
    elsif rising_edge(clk) then
      q_good <= d;
    end if;
  end process;
end architecture;
