(function () {
  const theme = document.documentElement.getAttribute("data-theme") || "light";
  if (window.hljs) {
    document.querySelectorAll("pre code").forEach(function (block) {
      if (block.parentElement && block.parentElement.classList.contains("mermaid"))
        return;
      hljs.highlightElement(block);
    });
  }
  if (window.mermaid && document.querySelector(".mermaid")) {
    mermaid.initialize({
      startOnLoad: false,
      securityLevel: "strict",
      theme: theme === "dark" ? "dark" : "neutral"
    });
    mermaid.run({ querySelector: ".mermaid" });
  }
})();
