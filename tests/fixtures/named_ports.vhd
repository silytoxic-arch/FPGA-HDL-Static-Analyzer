entity vhdl_port_child is
  port (A : in std_logic; Y : out std_logic);
end entity;

entity vhdl_port_parent is
  port (source_value : in std_logic; result_value : out std_logic);
end entity;

architecture rtl of vhdl_port_parent is begin
  u_bad : entity work.vhdl_port_child port map (
    a => source_value, Missing => result_value, A => source_value);
end architecture;
