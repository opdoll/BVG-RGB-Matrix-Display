FROM python:3.13-slim

WORKDIR /app

COPY server/pyproject.toml ./
RUN pip install --no-cache-dir .

COPY server/main.py ./

EXPOSE 8000

CMD ["fastapi", "run", "main.py", "--port", "8000"]
