FROM nginx:1.27-alpine

ENV BACKEND_PROXY=http://backend:8080

COPY public /usr/share/nginx/html
COPY nginx.conf /etc/nginx/conf.d/default.conf.template

EXPOSE 80

CMD sh -c "envsubst '\${BACKEND_PROXY}' < /etc/nginx/conf.d/default.conf.template > /etc/nginx/conf.d/default.conf && exec nginx -g 'daemon off;'"
