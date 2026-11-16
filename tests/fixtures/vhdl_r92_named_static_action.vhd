-- R-9-2 positive: an owner-entity generic is in the native per-root static
-- action set, so the following ELSIF edge establishes a class-1 clock name.
entity r92_named_static_action is
  generic(reset_value : bit := '0');
  port(clk, rst, d : in bit; q1, q2 : out bit);
end entity;

architecture rtl of r92_named_static_action is
begin
  source_proc: process(clk, rst)
  begin
    if rst = '0' then
      q1 <= reset_value;
    elsif rising_edge(clk) then
      q1 <= d;
    end if;
  end process;

  control_proc: process(clk, d)
  begin
    if clk then
      q2 <= d;
    end if;
  end process;
end architecture;
