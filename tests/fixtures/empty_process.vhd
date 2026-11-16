entity empty_process is
  port (a : in bit; q : out bit);
end entity;

architecture rtl of empty_process is
begin
  empty_body : process(all)
  begin
    null;
  end process;
  q <= a;
end architecture;
