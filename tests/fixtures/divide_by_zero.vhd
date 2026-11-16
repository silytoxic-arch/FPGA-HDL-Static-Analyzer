entity divide_by_zero is
  port (data : in integer; q : out integer);
end entity;
architecture rtl of divide_by_zero is begin
  process(all) begin
    q <= data / 0;
    q <= data mod (0);
    q <= data rem ((0));
    q <= data / 1;
    q <= data / (0 + 1);
    if data /= 0 then q <= data mod 3; end if;
  end process;
end architecture;
