entity duplicate_condition is
  port (
    condition_value : in  boolean;
    q               : out boolean
  );
end entity;

architecture rtl of duplicate_condition is
begin
  process(all)
  begin
    q <= false;
    if condition_value then
      q <= true;
    elsif (CONDITION_VALUE) then
      q <= false;
    end if;
  end process;
end architecture;
