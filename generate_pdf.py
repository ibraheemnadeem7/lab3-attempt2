import markdown
import weasyprint

with open("report.md", "r") as f:
    md_text = f.read()

html_body = markdown.markdown(
    md_text,
    extensions=["tables", "fenced_code", "codehilite"]
)

html = f"""<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<style>
  body {{
    font-family: Georgia, 'Times New Roman', serif;
    font-size: 11pt;
    line-height: 1.6;
    max-width: 780px;
    margin: 40px auto;
    padding: 0 40px;
    color: #1a1a1a;
  }}
  h1 {{
    font-size: 20pt;
    border-bottom: 2px solid #333;
    padding-bottom: 8px;
    margin-top: 0;
  }}
  h2 {{
    font-size: 15pt;
    border-bottom: 1px solid #aaa;
    padding-bottom: 4px;
    margin-top: 36px;
    color: #222;
  }}
  h3 {{
    font-size: 12.5pt;
    margin-top: 24px;
    color: #333;
  }}
  h4 {{
    font-size: 11.5pt;
    font-style: italic;
    color: #444;
    margin-top: 18px;
    margin-bottom: 4px;
  }}
  p {{
    margin: 10px 0;
    text-align: justify;
  }}
  code {{
    font-family: 'Courier New', monospace;
    background: #f4f4f4;
    padding: 1px 4px;
    border-radius: 3px;
    font-size: 9.5pt;
  }}
  pre {{
    background: #f4f4f4;
    border-left: 4px solid #999;
    padding: 12px 16px;
    overflow-x: auto;
    font-size: 9.5pt;
    line-height: 1.4;
    margin: 14px 0;
  }}
  pre code {{
    background: none;
    padding: 0;
  }}
  table {{
    border-collapse: collapse;
    width: 100%;
    margin: 18px 0;
    font-size: 10pt;
  }}
  th {{
    background-color: #333;
    color: white;
    padding: 8px 12px;
    text-align: center;
    font-weight: bold;
  }}
  td {{
    border: 1px solid #ccc;
    padding: 7px 12px;
    text-align: center;
  }}
  tr:nth-child(even) td {{
    background-color: #f9f9f9;
  }}
  tr:first-child td {{
    font-weight: normal;
  }}
  ul, ol {{
    margin: 10px 0;
    padding-left: 28px;
  }}
  li {{
    margin: 4px 0;
  }}
  hr {{
    border: none;
    border-top: 1px solid #ccc;
    margin: 30px 0;
  }}
  blockquote {{
    border-left: 4px solid #ccc;
    margin: 14px 0;
    padding: 2px 16px;
    color: #555;
  }}
  strong {{
    color: #111;
  }}
</style>
</head>
<body>
{html_body}
</body>
</html>"""

weasyprint.HTML(string=html).write_pdf("report.pdf")
print("report.pdf generated successfully")
