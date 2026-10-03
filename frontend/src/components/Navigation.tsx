export type Page =
  | "mission"
  | "crew"
  | "map"
  | "comms"
  | "log";

interface NavigationProps {
  activePage: Page;
  onChange: (page: Page) => void;
}

const navigation: {
  id: Page;
  label: string;
}[] = [
  { id: "mission", label: "MISSION LIVE" },
  { id: "crew", label: "CREW" },
  { id: "map", label: "LUNAR MAP" },
  { id: "comms", label: "COMMS" },
  { id: "log", label: "MISSION LOG" },
];

export function Navigation({
  activePage,
  onChange,
}: NavigationProps) {
  return (
    <nav className="navigation">
      <div className="aims-container nav-inner">
        {navigation.map((item) => (
          <button
            key={item.id}
            className={
              activePage === item.id
                ? "nav-button active"
                : "nav-button"
            }
            onClick={() => onChange(item.id)}
          >
            {item.label}
          </button>
        ))}
      </div>
    </nav>
  );
}