entity association_child is
  generic (WIDTH : positive := 8; MODE : natural := 0);
  port (a : in std_logic; b : in std_logic; y : out std_logic);
end entity;

entity association_top is
  port (a : in std_logic; y : out std_logic);
end entity;

architecture rtl of association_top is
begin
  u_overlap : entity work.association_child
    generic map (8, WIDTH => 16)
    port map (a, a => a, b => a, y => y);

  u_order : entity work.association_child
    generic map (MODE => 1, 8)
    port map (b => a, a, y => y);
end architecture;
