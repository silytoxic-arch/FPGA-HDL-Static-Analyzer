
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_ARITH.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

-------------------------------------------------------------------------------
entity utest_GJB_4_10_h_ecode is
-------------------------------------------------------------------------------
port(
    clk  : in  std_logic;
    rst  : in  std_logic;
	din  : in  std_logic;
    dout : out std_logic
);
-------------------------------------------------------------------------------
end entity utest_GJB_4_10_h_ecode;
-------------------------------------------------------------------------------

-------------------------------------------------------------------------------
architecture utest_GJB_4_10_h_ecode_arch of utest_GJB_4_10_h_ecode is
-------------------------------------------------------------------------------
begin

process (clk) 
begin
  if rising_edge(clk) then
      dout <= '0';           --违反
  end if;
end process;

-------------------------------------------------------------------------------
end architecture utest_GJB_4_10_h_ecode_arch;
-------------------------------------------------------------------------------